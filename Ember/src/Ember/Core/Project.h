#pragma once

#include "Core.h"

#include "ScopedPointer.h"
#include "SharedPointer.h"
#include "FilterManager.h"

#include <filesystem>
#include <string>

namespace Ember {

	struct ProjectConfig
	{
		std::string ProjectName = "NewProject";
		std::string EngineVersion = "0.1.0";
		std::string AssetDirectory = "GameData/Assets";
		std::string SceneDirectory = "GameData/Scenes";
		std::string StartScene = "Default.ebs";
		std::string AssetFile = "Assets.eba";

		ProjectConfig() = default;
		ProjectConfig(const std::string& projectName) : ProjectName(projectName) {}
	};

	class Project : public SharedResource
	{
	public:
		Project(const std::string& projectPath) :
			m_ProjectDirectory(std::filesystem::path(projectPath).parent_path()),
			m_Config({ std::filesystem::path(projectPath).stem().string() }),
			m_CollisionFilterManager(ScopedPtr<FilterManager>::Create()),
			m_RenderFilterManager(ScopedPtr<FilterManager>::Create())
		{
		}

		~Project() = default;

		inline const ProjectConfig& GetConfig() const { return m_Config; }

		inline std::filesystem::path GetProjectDirectory() const { return m_ProjectDirectory; }
		inline std::filesystem::path GetProjectFilePath() const { return std::filesystem::path(m_ProjectDirectory) / (m_Config.ProjectName + ".ebproj"); }
		inline std::filesystem::path GetAssetDirectory() const { return std::filesystem::path(m_ProjectDirectory) / m_Config.AssetDirectory; }
		inline std::filesystem::path GetScenesDirectory() const { return std::filesystem::path(m_ProjectDirectory) / m_Config.SceneDirectory; }

		inline std::filesystem::path GetAssetsFilePath() const { return GetAssetDirectory() / m_Config.AssetFile; }
		inline std::filesystem::path GetStartScenePath() const { return GetScenesDirectory() / m_Config.StartScene; }

		inline FilterManager& GetCollisionFilterManager() { return *m_CollisionFilterManager.Ptr(); }
		inline FilterManager& GetRenderFilterManager() { return *m_RenderFilterManager.Ptr(); }


	private:
		ProjectConfig m_Config;
		std::filesystem::path m_ProjectDirectory;

		ScopedPtr<FilterManager> m_CollisionFilterManager = nullptr;
		ScopedPtr<FilterManager> m_RenderFilterManager = nullptr;

		friend class ProjectSerializer;
	};

}