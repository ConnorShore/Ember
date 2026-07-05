#include "ebpch.h"

#include "NavigationMeshSerializer.h"
#include "AssetSerializationMode.h"

#include <ryml.hpp>
#include <ryml_std.hpp>

namespace Ember {

	namespace {
		template<typename T>
		void WriteRaw(std::ofstream& stream, const T& value)
		{
			stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
		}

		template<typename T>
		bool ReadRaw(std::ifstream& stream, T& value)
		{
			stream.read(reinterpret_cast<char*>(&value), sizeof(T));
			return stream.good();
		}

		constexpr uint32_t NAV_MESH_SOURCE_FILE_VERSION = 1;
		constexpr uint32_t NAV_MESH_COOKED_MAGIC = 0x4E41564D; // "NAVM"
		constexpr uint32_t NAV_MESH_COOKED_VERSION = 1;

		std::filesystem::path GetSourcePath(const std::filesystem::path& filepath)
		{
			auto sourcePath = filepath;
			sourcePath.replace_extension(".ebnav");
			return sourcePath;
		}

		std::filesystem::path GetBlobPath(const std::filesystem::path& filepath)
		{
			return GetSourcePath(filepath).string() + ".blob";
		}

		std::filesystem::path GetCookedPath(const std::filesystem::path& filepath)
		{
			auto cookedPath = filepath;
			cookedPath.replace_extension(".bin");
			return cookedPath;
		}
	}

	bool NavigationMeshSerializer::SerializeSource(const std::filesystem::path& filepath, const SharedPtr<NavigationMeshData>& navMesh)
	{
		if (!navMesh)
			return false;

		auto outputPath = GetSourcePath(filepath);
		auto blobPath = GetBlobPath(filepath);

		const auto& settings = navMesh->GetBakeSettings();

		ryml::Tree tree;
		ryml::NodeRef root = tree.rootref();
		root |= ryml::MAP;
		root["Version"] << NAV_MESH_SOURCE_FILE_VERSION;
		root["NavMeshData"] << navMesh->GetName();
		root["UUID"] << (uint64_t)navMesh->GetUUID();

		ryml::NodeRef settingsNode = root["BakeSettings"];
		settingsNode |= ryml::MAP;
		settingsNode["CellSize"] << settings.CellSize;
		settingsNode["CellHeight"] << settings.CellHeight;
		settingsNode["AgentHeight"] << settings.AgentHeight;
		settingsNode["AgentRadius"] << settings.AgentRadius;
		settingsNode["AgentMaxClimb"] << settings.AgentMaxClimb;
		settingsNode["AgentMaxSlope"] << settings.AgentMaxSlope;

		root["BlobFile"] << blobPath.filename().string();
		root["BlobSize"] << (uint64_t)navMesh->GetRawDataBlob().size();

		std::ofstream yamlOut(outputPath);
		if (!yamlOut.is_open())
			return false;
		yamlOut << tree;
		yamlOut.close();

		std::ofstream blobOut(blobPath, std::ios::binary | std::ios::trunc);
		if (!blobOut.is_open())
			return false;

		const auto& blob = navMesh->GetRawDataBlob();
		if (!blob.empty())
			blobOut.write(reinterpret_cast<const char*>(blob.data()), static_cast<std::streamsize>(blob.size()));

		return blobOut.good();
	}

	SharedPtr<NavigationMeshData> NavigationMeshSerializer::DeserializeSource(UUID uuid, const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		if (!stream.is_open())
		{
			EB_CORE_ERROR("Failed to open nav mesh source file: {0}", filepath.string());
			return nullptr;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();
		std::string yamlData = strStream.str();
		stream.close();

		ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yamlData));
		ryml::NodeRef root = tree.rootref();

		NavigationMeshBakeSettings settings;
		if (root.has_child("BakeSettings"))
		{
			auto settingsNode = root["BakeSettings"];
			if (settingsNode.has_child("CellSize")) settingsNode["CellSize"] >> settings.CellSize;
			if (settingsNode.has_child("CellHeight")) settingsNode["CellHeight"] >> settings.CellHeight;
			if (settingsNode.has_child("AgentHeight")) settingsNode["AgentHeight"] >> settings.AgentHeight;
			if (settingsNode.has_child("AgentRadius")) settingsNode["AgentRadius"] >> settings.AgentRadius;
			if (settingsNode.has_child("AgentMaxClimb")) settingsNode["AgentMaxClimb"] >> settings.AgentMaxClimb;
			if (settingsNode.has_child("AgentMaxSlope")) settingsNode["AgentMaxSlope"] >> settings.AgentMaxSlope;
		}

		std::filesystem::path blobPath = GetBlobPath(filepath);
		if (root.has_child("BlobFile"))
		{
			std::string blobFile;
			root["BlobFile"] >> blobFile;
			if (!blobFile.empty())
				blobPath = filepath.parent_path() / blobFile;
		}

		std::vector<uint8_t> blob;
		std::ifstream blobIn(blobPath, std::ios::binary);
		if (blobIn.is_open())
		{
			blobIn.seekg(0, std::ios::end);
			std::streamsize size = blobIn.tellg();
			blobIn.seekg(0, std::ios::beg);

			if (size > 0)
			{
				blob.resize(static_cast<size_t>(size));
				blobIn.read(reinterpret_cast<char*>(blob.data()), size);
				if (!blobIn.good() && !blobIn.eof())
					return nullptr;
			}
		}

		auto navMesh = SharedPtr<NavigationMeshData>::Create(uuid, filepath.stem().string(), filepath.string());
		navMesh->SetBakeSettings(settings);
		navMesh->SetRawDataBlob(std::move(blob));
		return navMesh;
	}

	bool NavigationMeshSerializer::SerializeCooked(const std::filesystem::path& filepath, const SharedPtr<NavigationMeshData>& navMesh)
	{
		if (!navMesh)
			return false;

		auto cookedPath = GetCookedPath(filepath);
		std::ofstream out(cookedPath, std::ios::binary | std::ios::trunc);
		if (!out.is_open())
			return false;

		const auto& settings = navMesh->GetBakeSettings();
		const auto& blob = navMesh->GetRawDataBlob();

		WriteRaw(out, NAV_MESH_COOKED_MAGIC);
		WriteRaw(out, NAV_MESH_COOKED_VERSION);

		WriteRaw(out, settings.CellSize);
		WriteRaw(out, settings.CellHeight);
		WriteRaw(out, settings.AgentHeight);
		WriteRaw(out, settings.AgentRadius);
		WriteRaw(out, settings.AgentMaxClimb);
		WriteRaw(out, settings.AgentMaxSlope);

		uint64_t blobSize = static_cast<uint64_t>(blob.size());
		WriteRaw(out, blobSize);
		if (blobSize > 0)
			out.write(reinterpret_cast<const char*>(blob.data()), static_cast<std::streamsize>(blobSize));

		return out.good();
	}

	SharedPtr<NavigationMeshData> NavigationMeshSerializer::DeserializeCooked(UUID uuid, const std::filesystem::path& filepath)
	{
		std::ifstream in(filepath, std::ios::binary);
		if (!in.is_open())
			return nullptr;

		uint32_t magic = 0;
		uint32_t version = 0;
		if (!ReadRaw(in, magic) || !ReadRaw(in, version))
			return nullptr;

		if (magic != NAV_MESH_COOKED_MAGIC || version > NAV_MESH_COOKED_VERSION)
			return nullptr;

		NavigationMeshBakeSettings settings;
		if (!ReadRaw(in, settings.CellSize)) return nullptr;
		if (!ReadRaw(in, settings.CellHeight)) return nullptr;
		if (!ReadRaw(in, settings.AgentHeight)) return nullptr;
		if (!ReadRaw(in, settings.AgentRadius)) return nullptr;
		if (!ReadRaw(in, settings.AgentMaxClimb)) return nullptr;
		if (!ReadRaw(in, settings.AgentMaxSlope)) return nullptr;

		uint64_t blobSize = 0;
		if (!ReadRaw(in, blobSize))
			return nullptr;

		std::vector<uint8_t> blob;
		if (blobSize > 0)
		{
			blob.resize(static_cast<size_t>(blobSize));
			in.read(reinterpret_cast<char*>(blob.data()), static_cast<std::streamsize>(blobSize));
			if (!in.good() && !in.eof())
				return nullptr;
		}

		auto navMesh = SharedPtr<NavigationMeshData>::Create(uuid, filepath.stem().string(), filepath.string());
		navMesh->SetBakeSettings(settings);
		navMesh->SetRawDataBlob(std::move(blob));
		return navMesh;
	}

	bool NavigationMeshSerializer::Serialize(const std::filesystem::path& filepath, const SharedPtr<NavigationMeshData>& mesh)
	{
		return SerializeSource(filepath, mesh);
	}

	SharedPtr<NavigationMeshData> NavigationMeshSerializer::Deserialize(UUID uuid, const std::filesystem::path& filepath)
	{
		switch (AssetSerializationMode::GetRuntimeLoadTier())
		{
		case RuntimeAssetLoadTier::ForceSourceYaml:
			return DeserializeSource(uuid, filepath);
		case RuntimeAssetLoadTier::ForceCookedBinary:
			return DeserializeCooked(uuid, GetCookedPath(filepath));
		case RuntimeAssetLoadTier::Auto:
		default:
			if (filepath.extension() == ".bin")
				return DeserializeCooked(uuid, filepath);
			return DeserializeSource(uuid, filepath);
		}
	}

}