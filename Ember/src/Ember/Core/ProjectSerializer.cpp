#include "ebpch.h"
#include "ProjectSerializer.h"

#include <ryml.hpp>
#include <ryml_std.hpp>
#include <fstream>
#include <sstream>

namespace Ember {

	bool ProjectSerializer::Serialize(const std::string& filePath)
	{
		ryml::Tree tree;
		ryml::NodeRef root = tree.rootref();
		root |= ryml::MAP;

		root["Project"] << m_Project->GetConfig().ProjectName;
		root["EngineVersion"] << m_Project->GetConfig().EngineVersion;
		root["StartScene"] << m_Project->GetConfig().StartScene;
		root["AssetDirectory"] << m_Project->GetConfig().AssetDirectory;

		std::ofstream fout(filePath);
		fout << tree;
		fout.close();

		return true;
	}

	bool ProjectSerializer::Deserialize(const std::string& filePath)
	{
		std::ifstream stream(filePath);
		if (!stream.is_open())
		{
			EB_CORE_ERROR("Failed to open scene file: {0}", filePath);
			return false;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();
		std::string yamlData = strStream.str();

		ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yamlData));
		ryml::NodeRef root = tree.rootref();

		if (!root.has_child("Project"))
			return false;

		auto& config = m_Project->m_Config;
		root["Project"] >> config.ProjectName;
		root["EngineVersion"] >> config.EngineVersion;
		root["StartScene"] >> config.StartScene;
		root["AssetDirectory"] >> config.AssetDirectory;

		m_Project->m_ProjectDirectory = std::filesystem::path(filePath).parent_path();

		return true;
	}

}