#pragma once

#include "ScopedPointer.h"
#include "SharedPointer.h"

#include "Ember/Physics/CollisionFilterManager.h"

#include <filesystem>
#include <string>

namespace Ember {

	struct ProjectConfig
	{
		std::string ProjectName = "NewProject";
		std::string EngineVersion = "0.1.0";
		std::string StartScene = "Scenes/Default.ebs";
		std::string AssetDirectory = "Assets";

		ProjectConfig() = default;
		ProjectConfig(const std::string& projectName) : ProjectName(projectName) {}
	};

	class Project : public SharedResource
	{
	public:
		Project(const std::string& projectPath) :
			m_ProjectDirectory(std::filesystem::path(projectPath).parent_path()),
			m_Config({ std::filesystem::path(projectPath).stem().string() }),
			m_CollisionFilterManager(ScopedPtr<CollisionFilterManager>::Create())
		{
		}

		~Project() = default;

		inline const ProjectConfig& GetConfig() const { return m_Config; }

		inline std::filesystem::path GetProjectDirectory() const { return m_ProjectDirectory; }
		inline std::filesystem::path GetAssetDirectory() const { return std::filesystem::path(m_ProjectDirectory) / m_Config.AssetDirectory; }
		inline std::filesystem::path GetStartScenePath() const { return std::filesystem::path(m_ProjectDirectory) / m_Config.StartScene; }

		inline CollisionFilterManager& GetCollisionFilterManager() { return *m_CollisionFilterManager.Ptr(); }
		inline const CollisionFilterManager& GetCollisionFilterManager() const { return *m_CollisionFilterManager.Ptr(); }

	private:
		ProjectConfig m_Config;
		std::filesystem::path m_ProjectDirectory;

		ScopedPtr<CollisionFilterManager> m_CollisionFilterManager = nullptr;

		friend class ProjectSerializer;
	};

}