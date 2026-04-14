#include "ebpch.h"
#include "ProjectSerializer.h"

#include "Ember/Utils/SerializationUtils.h"
#include "Ember/ECS/System/PhysicsSystem.h"

#include <ryml.hpp>
#include <ryml_std.hpp>
#include <vector>
#include <string>
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

		// Settings
		auto settingsNode = root["Settings"];
		settingsNode |= ryml::MAP;

		// Physics Settings
		auto& physicsSettings = Application::Instance().GetSystem<PhysicsSystem>()->GetSettings();
		auto physicsNode = root["Settings"]["Physics"];
		physicsNode |= ryml::MAP;

		physicsNode["GravityStrength"] << physicsSettings.GravityStrength;
		ryml::NodeRef gravityVectorNode = physicsNode["GravityVector"];
		Util::SerializeVector3f(gravityVectorNode, physicsSettings.GravityVector);
		physicsNode["UpdateRate"] << physicsSettings.UpdateRate;
		physicsNode["PositionSolverIterations"] << physicsSettings.PositionSolverIterations;
		physicsNode["VelocitySolverIterations"] << physicsSettings.VelocitySolverIterations;

		// Physics Collider Filters
		auto& filterManager = m_Project->GetCollisionFilterManager();
		auto filterNode = physicsNode["CollisionFilters"];
		filterNode |= ryml::SEQ;
		for (const auto& filter : filterManager.GetCustomFilters())
		{
			auto filterEntryNode = filterNode.append_child();
			filterEntryNode |= ryml::MAP;
			filterEntryNode["Name"] << filter;
		}

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

		// Settings
		if (root.has_child("Settings"))
		{
			auto settingsNode = root["Settings"];

			// Physics Settings
			auto& physicsSettings = Application::Instance().GetSystem<PhysicsSystem>()->GetSettings();
			auto physicsNode = settingsNode["Physics"];
			physicsNode["GravityStrength"] >> physicsSettings.GravityStrength;
			auto gravityVectorNode = physicsNode["GravityVector"];
			Util::DeserializeVector3f(gravityVectorNode, physicsSettings.GravityVector);
			physicsNode["UpdateRate"] >> physicsSettings.UpdateRate;
			physicsNode["PositionSolverIterations"] >> physicsSettings.PositionSolverIterations;
			physicsNode["VelocitySolverIterations"] >> physicsSettings.VelocitySolverIterations;

			// Physics Collider Filters
			auto& filterManager = m_Project->GetCollisionFilterManager();
			std::vector<std::string> customFilters;

			for (auto filterNode : physicsNode["CollisionFilters"].children())
			{
				std::string filterName;
				filterNode["Name"] >> filterName;
				customFilters.push_back(filterName);
			}

			filterManager.InitWithCustomFilters(customFilters);
		}

		m_Project->m_ProjectDirectory = std::filesystem::path(filePath).parent_path();

		return true;
	}

}