#include "efpch.h"
#include "RecentProjectSerializer.h"

#include <ryml.hpp>
#include <ryml_std.hpp>

namespace Ember {

	RecentProjectSerializer::RecentProjectSerializer(std::vector<RecentProject>& recentProjects)
		: m_RecentProjects(recentProjects)
	{
	}

	bool RecentProjectSerializer::Serialize(const std::filesystem::path filePath)
	{
		ryml::Tree tree;
		ryml::NodeRef root = tree.rootref();
		root |= ryml::MAP;

		auto recentProjectsNode = root["RecentProjects"];
		recentProjectsNode |= ryml::SEQ;

		for (const auto& recentProj : m_RecentProjects)
		{
			auto recentProjNode = recentProjectsNode.append_child();
			recentProjNode |= ryml::MAP;
			recentProjNode["Name"] << recentProj.Name;
			recentProjNode["Path"] << recentProj.Path;
		}
		
		std::ofstream fout(filePath);
		if (!fout.is_open())
		{
			EB_CORE_ERROR("Failed to open recent project file for writing: {}", filePath.string());
			return false;
		}

		fout << tree;
		fout.close();

		return true;
	}

	bool RecentProjectSerializer::Deserialize(const std::filesystem::path filePath)
	{
		std::ifstream stream(filePath);
		if (!stream.is_open())
		{
			EB_CORE_ERROR("Failed to open recent project file: {}", filePath.string());
			return false;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();
		std::string yamlData = strStream.str();

		ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yamlData));
		ryml::NodeRef root = tree.rootref();

		if (!root.has_child("RecentProjects"))
			return false;

		auto recentProjectsNode = root["RecentProjects"];
		m_RecentProjects.clear();
		m_RecentProjects.reserve(recentProjectsNode.num_children());
		for (auto recentProjNode : recentProjectsNode.children())
		{
			if (!recentProjNode.is_map() || !recentProjNode.has_child("Name") || !recentProjNode.has_child("Path"))
				continue;

			RecentProject recentProj;
			recentProjNode["Name"] >> recentProj.Name;
			recentProjNode["Path"] >> recentProj.Path;
			m_RecentProjects.push_back(std::move(recentProj));
		}

		return true;
	}

}