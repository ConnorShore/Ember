#include "ebpch.h"
#include "SaveGameManager.h"
#include "Ember/Core/ProjectManager.h"

#include <ryml.hpp> 
#include <ryml_std.hpp>

namespace Ember {

	int SaveGameManager::GetInt(const std::string& key, int defaultValue)
	{
		if (m_IntData.find(key) != m_IntData.end())
			return m_IntData[key];
		return defaultValue;
	}

	float SaveGameManager::GetFloat(const std::string& key, float defaultValue)
	{
		if (m_FloatData.find(key) != m_FloatData.end())
			return m_FloatData[key];
		return defaultValue;
	}

	std::string SaveGameManager::GetString(const std::string& key, const std::string& defaultValue)
	{
		if (m_StringData.find(key) != m_StringData.end())
			return m_StringData[key];
		return defaultValue;
	}

	std::filesystem::path SaveGameManager::GetOSSaveDirectory() const
	{
		std::filesystem::path saveDir;

#ifdef EB_PLATFORM_WINDOWS
		// Grab the local AppData folder safely without pulling in heavy Windows headers
		char* appDataPath = nullptr;
		size_t size = 0;
		if (_dupenv_s(&appDataPath, &size, "LOCALAPPDATA") == 0 && appDataPath != nullptr)
		{
			std::string projectName = "EmberGame";
			if (ProjectManager::GetActive())
				projectName = ProjectManager::GetActive()->GetConfig().ProjectName;

			saveDir = std::filesystem::path(appDataPath) / projectName / "SavedGames";
			free(appDataPath);
		}
#endif
		// Ensure the directory actually exists before we try to write to it!
		if (!std::filesystem::exists(saveDir))
			std::filesystem::create_directories(saveDir);

		return saveDir;
	}

	bool SaveGameManager::SaveToFile(const std::string& filename)
	{
		std::filesystem::path filepath = GetOSSaveDirectory() / filename;
		EB_CORE_INFO("Saving game to: {0}", filepath.string());

		ryml::Tree tree;
		ryml::NodeRef root = tree.rootref();
		root |= ryml::MAP;

		auto saveDataNode = root["SaveData"];
		saveDataNode |= ryml::MAP;

		// Int data
		auto intDataNode = saveDataNode["IntData"];
		intDataNode |= ryml::SEQ;

		for (const auto& [key, value] : m_IntData)
		{
			auto entryNode = intDataNode.append_child();
			entryNode |= ryml::MAP;
			entryNode["Key"] << key;
			entryNode["Value"] << value;
		}

		// Float data
		auto floatDataNode = saveDataNode["FloatData"];
		floatDataNode |= ryml::SEQ;
		for (const auto& [key, value] : m_FloatData)
		{
			auto entryNode = floatDataNode.append_child();
			entryNode |= ryml::MAP;
			entryNode["Key"] << key;
			entryNode["Value"] << value;
		}

		// String data
		auto stringDataNode = saveDataNode["StringData"];
		stringDataNode |= ryml::SEQ;
		for (const auto& [key, value] : m_StringData)
		{
			auto entryNode = stringDataNode.append_child();
			entryNode |= ryml::MAP;
			entryNode["Key"] << key;
			entryNode["Value"] << value;
		}

		std::ofstream fout(filepath);
		fout << tree;
		fout.close();

		return true;
	}

	bool SaveGameManager::LoadFromFile(const std::string& filename)
	{
		std::filesystem::path filepath = GetOSSaveDirectory() / filename;
		if (!std::filesystem::exists(filepath))
		{
			EB_CORE_WARN("Save file does not exist: {0}", filepath.string());
			return false;
		}

		EB_CORE_INFO("Loading game from: {0}", filepath.string());

		// Clear current state
		m_IntData.clear();
		m_FloatData.clear();
		m_StringData.clear();

		// Parse in game data
		std::ifstream stream(filepath);
		if (!stream.is_open())
		{
			EB_CORE_ERROR("Failed to open save game file: {0}", filepath.string());
			return false;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();
		std::string yamlData = strStream.str();

		ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yamlData));
		ryml::NodeRef root = tree.rootref();

		if (!root.has_child("SaveData"))
		{
			EB_CORE_ERROR("Invalid save game file format: missing SaveData node");
			return false;
		}

		auto saveDataNode = root["SaveData"];

		// Int data
		if (saveDataNode.has_child("IntData"))
		{
			for (auto entryNode : saveDataNode["IntData"].children())
			{
				std::string key;
				int value;
				entryNode["Key"] >> key;
				entryNode["Value"] >> value;
				m_IntData[key] = value;
			}
		}

		// FLoat data
		if (saveDataNode.has_child("FloatData"))
		{
			for (auto entryNode : saveDataNode["FloatData"].children())
			{
				std::string key;
				float value;
				entryNode["Key"] >> key;
				entryNode["Value"] >> value;
				m_FloatData[key] = value;
			}
		}

		// String data
		if (saveDataNode.has_child("StringData"))
		{
			for (auto entryNode : saveDataNode["StringData"].children())
			{
				std::string key;
				std::string value;
				entryNode["Key"] >> key;
				entryNode["Value"] >> value;
				m_StringData[key] = value;
			}
		}

		return true;
	}
}