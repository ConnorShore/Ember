#include "ebpch.h"
#include "PhysicsMaterialSerializer.h"
#include "Ember/Utils/SerializationUtils.h"

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
}

namespace Ember {
	namespace {
		constexpr uint32_t PHYS_MAT_SOURCE_FILE_VERSION = 1;
		constexpr uint32_t PHYS_MAT_COOKED_MAGIC = 0x4542504D; // EBPM
		constexpr uint32_t PHYS_MAT_COOKED_VERSION = 1;

		std::filesystem::path GetCookedPath(const std::filesystem::path& filepath)
		{
			auto cookedPath = filepath;
			cookedPath.replace_extension(".bin");
			return cookedPath;
		}
	}

	bool PhysicsMaterialSerializer::SerializeSource(const std::filesystem::path& filepath, const SharedPtr<PhysicsMaterial>& material)
	{
		ryml::Tree tree;
		ryml::NodeRef root = tree.rootref();
		root |= ryml::MAP;
		root["Version"] << PHYS_MAT_SOURCE_FILE_VERSION;

		root["Material"] << material->GetName();
		root["UUID"] << (uint64_t)material->GetUUID();

		// Properties
		ryml::NodeRef propertiesNode = root["Properties"];
		propertiesNode |= ryml::MAP;
		propertiesNode["Friction"] << material->Friction;
		propertiesNode["Bounciness"] << material->Bounciness;

		std::ofstream fout(filepath);
		fout << tree;
		fout.close();
		return true;
	}

	SharedPtr<PhysicsMaterial> PhysicsMaterialSerializer::DeserializeSource(UUID uuid, const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		if (!stream.is_open())
		{
			EB_CORE_ERROR("Failed to open material file: {0}", filepath.string());
			return nullptr;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();
		std::string yamlData = strStream.str();
		stream.close();

		ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yamlData));
		ryml::NodeRef root = tree.rootref();

		std::string name;
		root["Material"] >> name;

		// Properties
		ryml::NodeRef propertiesNode = root["Properties"];
		
		float friction = 0.5f;
		Util::ReadField(propertiesNode, "Friction", friction);

		float bounciness = 0.0f;
		Util::ReadField(propertiesNode, "Bounciness", bounciness);

		auto material = SharedPtr<PhysicsMaterial>::Create(uuid, name, filepath.string());
		material->Friction = friction;
		material->Bounciness = bounciness;

		return material;
	}

	bool PhysicsMaterialSerializer::SerializeCooked(const std::filesystem::path& filepath, const SharedPtr<PhysicsMaterial>& material)
	{
		if (!material)
			return false;

		auto cookedPath = GetCookedPath(filepath);
		std::ofstream stream(cookedPath, std::ios::binary | std::ios::trunc);
		if (!stream.is_open())
			return false;

		WriteRaw(stream, PHYS_MAT_COOKED_MAGIC);
		WriteRaw(stream, PHYS_MAT_COOKED_VERSION);
		WriteRaw(stream, material->Friction);
		WriteRaw(stream, material->Bounciness);
		return true;
	}

	SharedPtr<PhysicsMaterial> PhysicsMaterialSerializer::DeserializeCooked(UUID uuid, const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath, std::ios::binary);
		if (!stream.is_open())
			return nullptr;

		uint32_t magic = 0;
		uint32_t version = 0;
		if (!ReadRaw(stream, magic) || !ReadRaw(stream, version))
			return nullptr;
		if (magic != PHYS_MAT_COOKED_MAGIC || version > PHYS_MAT_COOKED_VERSION)
			return nullptr;

		float friction = 0.5f;
		float bounciness = 0.0f;
		ReadRaw(stream, friction);
		ReadRaw(stream, bounciness);

		auto material = SharedPtr<PhysicsMaterial>::Create(uuid, filepath.stem().string(), filepath.string());
		material->Friction = friction;
		material->Bounciness = bounciness;
		return material;
	}

	bool PhysicsMaterialSerializer::Serialize(const std::filesystem::path& filepath, const SharedPtr<PhysicsMaterial>& material)
	{
		return SerializeSource(filepath, material);
	}

	SharedPtr<PhysicsMaterial> PhysicsMaterialSerializer::Deserialize(UUID uuid, const std::filesystem::path& filepath)
	{
		switch (AssetSerializationMode::GetRuntimeLoadTier())
		{
		case RuntimeAssetLoadTier::ForceSourceYaml:
			return DeserializeSource(uuid, filepath);
		case RuntimeAssetLoadTier::ForceCookedBinary:
			if (auto cooked = DeserializeCooked(uuid, GetCookedPath(filepath)))
				return cooked;
			return DeserializeSource(uuid, filepath);
		case RuntimeAssetLoadTier::Auto:
		default:
			if (filepath.extension() == ".bin")
			{
				if (auto cooked = DeserializeCooked(uuid, filepath))
					return cooked;
			}
			return DeserializeSource(uuid, filepath);
		}
	}

}