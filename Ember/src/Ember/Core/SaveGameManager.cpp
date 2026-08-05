#include "ebpch.h"
#include "SaveGameManager.h"
#include "Ember/Core/Paths.h"
#include "Ember/Core/ProjectManager.h"

#include <ryml.hpp>
#include <ryml_std.hpp>

#include <stdexcept>

namespace Ember {

	namespace {

		constexpr uint32_t SAVE_FILE_VERSION = 2;

		std::string NodeToString(ryml::NodeRef node)
		{
			if (!node.has_val())
				return {};

			c4::csubstr val = node.val();
			if (!val.str)
				return {};

			return std::string(val.str, val.len);
		}

		bool WriteSaveFile(const SaveGameFile& file, const std::filesystem::path& filepath)
		{
			ryml::Tree tree;
			ryml::NodeRef root = tree.rootref();
			root |= ryml::MAP;

			root["Version"] << SAVE_FILE_VERSION;
			root["Name"] << file.GetName();

			auto valuesNode = root["Values"];
			valuesNode |= ryml::SEQ;

			for (const auto& [key, value] : file.GetValues())
			{
				auto entryNode = valuesNode.append_child();
				entryNode |= ryml::MAP;
				entryNode["Key"] << key;

				std::visit([&entryNode](const auto& typedValue) {
					using T = std::decay_t<decltype(typedValue)>;
					if constexpr (std::same_as<T, int>)
					{
						entryNode["Type"] << "Int";
						entryNode["Value"] << typedValue;
					}
					else if constexpr (std::same_as<T, float>)
					{
						entryNode["Type"] << "Float";
						entryNode["Value"] << typedValue;
					}
					else if constexpr (std::same_as<T, bool>)
					{
						// Written as a word rather than relying on ryml's bool conversion, which
						// round-trips as 0/1 and would be indistinguishable from an Int.
						entryNode["Type"] << "Bool";
						entryNode["Value"] << (typedValue ? "true" : "false");
					}
					else if constexpr (std::same_as<T, std::string>)
					{
						entryNode["Type"] << "String";
						entryNode["Value"] << typedValue;
					}
				}, value);
			}

			std::ofstream fout(filepath);
			if (!fout.is_open())
			{
				EB_CORE_ERROR("Failed to open save game file for writing: {0}", filepath.string());
				return false;
			}

			fout << tree;

			const bool succeeded = fout.good();
			fout.close();

			if (!succeeded)
				EB_CORE_ERROR("Failed to write save game file: {0}", filepath.string());

			return succeeded;
		}

		// Version 1 layout: three per-type sequences under a SaveData node. Merged into the single
		// value map so files written before multi-file support still load.
		bool ReadLegacySaveData(SaveGameFile& file, ryml::NodeRef root)
		{
			if (!root.has_child("SaveData"))
			{
				EB_CORE_ERROR("Invalid save game file format: missing SaveData node");
				return false;
			}

			auto saveDataNode = root["SaveData"];
			if (!saveDataNode.is_map())
			{
				EB_CORE_ERROR("Invalid save game file format: SaveData is not a map");
				return false;
			}

			if (saveDataNode.has_child("IntData"))
			{
				for (auto entryNode : saveDataNode["IntData"].children())
				{
					if (!entryNode.is_map() || !entryNode.has_child("Key") || !entryNode.has_child("Value"))
						continue;

					std::string key;
					int value = 0;
					entryNode["Key"] >> key;
					entryNode["Value"] >> value;
					file.SetValue(std::move(key), value);
				}
			}

			if (saveDataNode.has_child("FloatData"))
			{
				for (auto entryNode : saveDataNode["FloatData"].children())
				{
					if (!entryNode.is_map() || !entryNode.has_child("Key") || !entryNode.has_child("Value"))
						continue;

					std::string key;
					float value = 0.0f;
					entryNode["Key"] >> key;
					entryNode["Value"] >> value;
					file.SetValue(std::move(key), value);
				}
			}

			if (saveDataNode.has_child("StringData"))
			{
				for (auto entryNode : saveDataNode["StringData"].children())
				{
					if (!entryNode.is_map() || !entryNode.has_child("Key") || !entryNode.has_child("Value"))
						continue;

					std::string key;
					entryNode["Key"] >> key;
					file.SetValue(std::move(key), NodeToString(entryNode["Value"]));
				}
			}

			return true;
		}

		bool ReadSaveFile(SaveGameFile& file, const std::filesystem::path& filepath)
		{
			std::ifstream stream(filepath);
			if (!stream.is_open())
			{
				EB_CORE_ERROR("Failed to open save game file: {0}", filepath.string());
				return false;
			}

			std::stringstream strStream;
			strStream << stream.rdbuf();
			std::string yamlData = strStream.str();

			if (yamlData.empty())
			{
				EB_CORE_WARN("Save game file is empty: {0}", filepath.string());
				return false;
			}

			ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yamlData));
			ryml::NodeRef root = tree.rootref();

			// A truncated or hand-edited save can parse into something that is not a map, and
			// has_child() asserts inside ryml on a non-container node.
			if (!root.is_map())
			{
				EB_CORE_ERROR("Save game file is not a YAML map: {0}", filepath.string());
				return false;
			}

			// No Version node means this is a file written before multi-file support.
			if (!root.has_child("Version"))
				return ReadLegacySaveData(file, root);

			uint32_t version = 0;
			root["Version"] >> version;
			if (version > SAVE_FILE_VERSION)
			{
				EB_CORE_ERROR("Save game file {0} is version {1}, which this build cannot read (max {2})",
					filepath.string(), version, SAVE_FILE_VERSION);
				return false;
			}

			if (!root.has_child("Values") || !root["Values"].is_seq())
			{
				EB_CORE_ERROR("Invalid save game file format: missing Values node");
				return false;
			}

			for (auto entryNode : root["Values"].children())
			{
				if (!entryNode.is_map())
					continue;

				if (!entryNode.has_child("Key") || !entryNode.has_child("Type") || !entryNode.has_child("Value"))
					continue;

				std::string key;
				entryNode["Key"] >> key;

				const std::string type = NodeToString(entryNode["Type"]);
				auto valueNode = entryNode["Value"];

				if (type == "Int")
				{
					int value = 0;
					if (valueNode.has_val())
						valueNode >> value;
					file.SetValue(std::move(key), value);
				}
				else if (type == "Float")
				{
					float value = 0.0f;
					if (valueNode.has_val())
						valueNode >> value;
					file.SetValue(std::move(key), value);
				}
				else if (type == "Bool")
				{
					const std::string value = NodeToString(valueNode);
					file.SetValue(std::move(key), value == "true" || value == "1");
				}
				else if (type == "String")
				{
					file.SetValue(std::move(key), NodeToString(valueNode));
				}
				else
				{
					EB_CORE_WARN("Skipping save game entry '{0}' with unknown type '{1}'", key, type);
				}
			}

			return true;
		}

	}

	//////////////////////////////////////////////////////////////////////////
	// SaveFileHandle
	//////////////////////////////////////////////////////////////////////////

	bool SaveFileHandle::IsValid() const
	{
		if (!Owner || Index >= Owner->m_Slots.size())
			return false;

		const SaveGameManager::Slot& slot = Owner->m_Slots[Index];
		return slot.File && slot.Generation == Generation;
	}

	SaveGameFile& SaveFileHandle::Resolve() const
	{
		if (IsValid())
			return *Owner->m_Slots[Index].File;

		EB_CORE_ASSERT(false, "Used a save file handle whose file is no longer open (was it closed?).");
		throw std::runtime_error("Used a save file handle whose file is no longer open (was it closed?).");
	}

	//////////////////////////////////////////////////////////////////////////
	// Slot management
	//////////////////////////////////////////////////////////////////////////

	std::string_view SaveGameManager::NormalizeName(std::string_view name)
	{
		if (name.empty())
			return DefaultSlotName;

		const std::string_view extension = SaveFileExtension;
		if (name.size() > extension.size() && name.ends_with(extension))
			name.remove_suffix(extension.size());

		return name.empty() ? std::string_view(DefaultSlotName) : name;
	}

	uint32_t SaveGameManager::AcquireSlot(std::string_view name, bool* outCreated)
	{
		const std::string_view slotName = NormalizeName(name);

		auto it = m_SlotsByName.find(slotName);
		if (it != m_SlotsByName.end() && m_Slots[it->second].File)
		{
			if (outCreated)
				*outCreated = false;
			return it->second;
		}

		if (outCreated)
			*outCreated = true;

		// Reuse the name's existing slot index when it was closed rather than growing the vector,
		// so the generation bump from Close() keeps invalidating its old handles.
		if (it != m_SlotsByName.end())
		{
			m_Slots[it->second].File = ScopedPtr<SaveGameFile>::Create(std::string(slotName));
			return it->second;
		}

		const uint32_t index = static_cast<uint32_t>(m_Slots.size());
		m_Slots.push_back(Slot{ ScopedPtr<SaveGameFile>::Create(std::string(slotName)), 0u });
		m_SlotsByName.emplace(std::string(slotName), index);
		return index;
	}

	SaveFileHandle SaveGameManager::Open(std::string_view name)
	{
		bool created = false;
		const uint32_t index = AcquireSlot(name, &created);

		// Only the first open reads from disk; afterwards the in-memory copy is authoritative.
		if (created)
		{
			const std::filesystem::path filepath = GetSaveFilePath(name);
			if (std::filesystem::exists(filepath))
				ReadSaveFile(*m_Slots[index].File, filepath);
		}

		return SaveFileHandle{ this, index, m_Slots[index].Generation };
	}

	SaveFileHandle SaveGameManager::Reload(std::string_view name)
	{
		const uint32_t index = AcquireSlot(name);
		SaveGameFile& file = *m_Slots[index].File;
		file.Clear();

		const std::filesystem::path filepath = GetSaveFilePath(name);
		if (std::filesystem::exists(filepath))
			ReadSaveFile(file, filepath);
		else
			EB_CORE_WARN("Save file does not exist: {0}", filepath.string());

		return SaveFileHandle{ this, index, m_Slots[index].Generation };
	}

	bool SaveGameManager::Save(std::string_view name)
	{
		const uint32_t index = AcquireSlot(name);
		const std::filesystem::path filepath = GetSaveFilePath(name);

		EB_CORE_INFO("Saving game to: {0}", filepath.string());
		return WriteSaveFile(*m_Slots[index].File, filepath);
	}

	bool SaveGameManager::SaveAll()
	{
		bool allSucceeded = true;
		for (const Slot& slot : m_Slots)
		{
			if (!slot.File)
				continue;

			const std::filesystem::path filepath = GetSaveFilePath(slot.File->GetName());
			if (!WriteSaveFile(*slot.File, filepath))
				allSucceeded = false;
		}

		return allSucceeded;
	}

	void SaveGameManager::Close(std::string_view name)
	{
		auto it = m_SlotsByName.find(NormalizeName(name));
		if (it == m_SlotsByName.end())
			return;

		Slot& slot = m_Slots[it->second];
		slot.File.Reset();
		slot.Generation++;
	}

	void SaveGameManager::CloseAll()
	{
		for (Slot& slot : m_Slots)
		{
			if (!slot.File)
				continue;

			slot.File.Reset();
			slot.Generation++;
		}
	}

	bool SaveGameManager::IsOpen(std::string_view name) const
	{
		auto it = m_SlotsByName.find(NormalizeName(name));
		return it != m_SlotsByName.end() && m_Slots[it->second].File;
	}

	SaveGameFile* SaveGameManager::Find(std::string_view name)
	{
		auto it = m_SlotsByName.find(NormalizeName(name));
		if (it == m_SlotsByName.end())
			return nullptr;

		return m_Slots[it->second].File.Ptr();
	}

	bool SaveGameManager::ExistsOnDisk(std::string_view name) const
	{
		return std::filesystem::exists(GetSaveFilePath(name));
	}

	bool SaveGameManager::DeleteFromDisk(std::string_view name)
	{
		std::error_code error;
		return std::filesystem::remove(GetSaveFilePath(name), error);
	}

	//////////////////////////////////////////////////////////////////////////
	// Paths
	//////////////////////////////////////////////////////////////////////////

	std::filesystem::path SaveGameManager::GetOSSaveDirectory() const
	{
		// Keyed on the project name, not on Ember, so a shipped game owns its own save folder.
		std::string projectName = "EmberGame";
		if (ProjectManager::GetActive())
			projectName = ProjectManager::GetActive()->GetConfig().ProjectName;

		std::filesystem::path saveDir = Paths::LocalAppData() / projectName / "SavedGames";

		// Ensure the directory actually exists before we try to write to it!
		if (!std::filesystem::exists(saveDir))
			std::filesystem::create_directories(saveDir);

		return saveDir;
	}

	std::filesystem::path SaveGameManager::GetSaveFilePath(std::string_view name) const
	{
		return GetOSSaveDirectory() / (std::string(NormalizeName(name)) + SaveFileExtension);
	}

}
