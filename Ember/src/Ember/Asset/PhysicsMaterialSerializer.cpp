#include "ebpch.h"
#include "PhysicsMaterialSerializer.h"

namespace Ember {


	bool PhysicsMaterialSerializer::Serialize(const std::filesystem::path& filepath, const SharedPtr<PhysicsMaterial>& material)
	{
		ryml::Tree tree;
		ryml::NodeRef root = tree.rootref();
		root |= ryml::MAP;

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

	SharedPtr<PhysicsMaterial> PhysicsMaterialSerializer::Deserialize(UUID uuid, const std::filesystem::path& filepath)
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
		if (propertiesNode.has_child("Friction"))
			propertiesNode["Friction"] >> friction;

		float bounciness = 0.0f;
		if (propertiesNode.has_child("Bounciness"))
			propertiesNode["Bounciness"] >> bounciness;

		auto material = SharedPtr<PhysicsMaterial>::Create(uuid, name, filepath.string());
		material->Friction = friction;
		material->Bounciness = bounciness;

		return material;
	}

}