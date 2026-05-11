#pragma once

#include "Core.h"
#include "Application.h"
#include "FilterManager.h"

#include "Ember/Scene/Scene.h"

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

		inline ProjectConfig& GetConfig() { return m_Config; }

		inline std::filesystem::path GetProjectDirectory() const { return m_ProjectDirectory; }
		inline std::filesystem::path GetProjectFilePath() const { return std::filesystem::path(m_ProjectDirectory) / (m_Config.ProjectName + ".ebproj"); }
		inline std::filesystem::path GetAssetDirectory() const { return std::filesystem::path(m_ProjectDirectory) / m_Config.AssetDirectory; }
		inline std::filesystem::path GetScenesDirectory() const { return std::filesystem::path(m_ProjectDirectory) / m_Config.SceneDirectory; }

		inline std::filesystem::path GetAssetsFilePath() const { return GetAssetDirectory() / m_Config.AssetFile; }
		inline std::filesystem::path GetStartScenePath() const { return GetScenesDirectory() / m_Config.StartScene; }

		inline FilterManager& GetCollisionFilterManager() { return *m_CollisionFilterManager.Ptr(); }
		inline FilterManager& GetRenderFilterManager() { return *m_RenderFilterManager.Ptr(); }

		inline void SetStartScene(UUID sceneUUID)
		{
			auto scene = Application::Instance().GetAssetManager().GetAsset<Scene>(sceneUUID);
			if (!scene)
			{
				EB_CORE_ERROR("Cannot set start scene, scene doesn't exist {}", (uint64_t)sceneUUID);
				return;
			}

			m_Config.StartScene = std::filesystem::path(scene->GetFilePath()).filename().string();
		}

		inline std::vector<UUID>& GetScenesInBuild() { return m_ScenesInBuild; }
		inline void SetScenesInBuild(const std::vector<UUID>& scenes) { m_ScenesInBuild = scenes; }
		inline void AddSceneToBuild(const UUID& scene) { m_ScenesInBuild.push_back(scene); }
		inline void RemoveSceneFromBuild(const UUID& scene)
		{
			m_ScenesInBuild.erase(std::remove(m_ScenesInBuild.begin(), m_ScenesInBuild.end(), scene), m_ScenesInBuild.end());
		}
		inline UUID GetNextScene()
		{
			m_ActiveSceneIndex++;
			if (m_ActiveSceneIndex >= m_ScenesInBuild.size())
			{
				EB_CORE_ERROR("Tried to retrieve next scene which does not exist");
				return Constants::InvalidUUID;
			}

			return m_ScenesInBuild[m_ActiveSceneIndex];
		}
		inline bool IsLastScene() const { return m_ActiveSceneIndex >= m_ScenesInBuild.size() - 1; }
		inline void ResetSceneIndex() { m_ActiveSceneIndex = 0; }

	private:
		ProjectConfig m_Config;
		std::filesystem::path m_ProjectDirectory;

		ScopedPtr<FilterManager> m_CollisionFilterManager = nullptr;
		ScopedPtr<FilterManager> m_RenderFilterManager = nullptr;

		std::vector<UUID> m_ScenesInBuild;
		uint32_t m_ActiveSceneIndex = 0;

		friend class ProjectSerializer;
	};

}