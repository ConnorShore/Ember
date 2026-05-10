#include "ebpch.h"
#include "SceneManager.h"
#include "Scene.h"
#include "SceneSerializer.h"

#include "Ember/Core/ProjectManager.h"

namespace Ember {

	SharedPtr<Scene> SceneManager::CreateScene(const std::string& name /*= ""*/)
	{
		auto& assetManager = Application::Instance().GetAssetManager();
		auto sceneName = name.empty() ? "Untitled Scene" : name;
		auto fullScenePath = ProjectManager::GetActive()->GetScenesDirectory() / (sceneName + ".ebs");

		// Use Load so the path is registered in m_AssetPaths — this ensures ExecuteSceneSwap's
		// Load call de-duplicates correctly and returns this same instance instead of a new one.
		auto newScene = assetManager.Load<Scene>(sceneName, fullScenePath.string(), false);

		SceneSerializer serializer(newScene);
		if (!serializer.Serialize(fullScenePath.string()))
		{
			return nullptr;
		}

		// Register the new scene in the active project automatically so it gets included in builds (unless removed)
		ProjectManager::GetActive()->AddSceneToBuild(newScene->GetUUID());

		LoadScene(fullScenePath.string());
		return newScene;
	}

	SharedPtr<Scene> SceneManager::GetActiveScene()
	{
		return m_ActiveScene;
	}

	void SceneManager::SetActiveScene(SharedPtr<Scene> scene)
	{
		if (m_ActiveScene == scene)
			return;

		if (m_ActiveScene)
			m_ActiveScene->OnDetach();

		m_ActiveScene = scene;

		if (m_ActiveScene)
			m_ActiveScene->OnAttach();
	}

	void SceneManager::LoadScene(const std::string& filepath)
	{
		if (filepath.empty())
		{
			EB_CORE_ERROR("Attempted to load scene with empty file path!");
			return;
		}
		if (m_ActiveScene->GetFilePath() == filepath)
		{
			EB_CORE_INFO("Scene {} is already active, skipping load request.", filepath);
			return;
		}

		// Don't load it now! Just remember we need to do it.
		m_NextScenePath = filepath;
		m_LoadRequested = true;
	}

	void SceneManager::LoadScene(UUID sceneUUID)
	{
		auto& assetManager = Application::Instance().GetAssetManager();

		// (Assuming you add a helper to get a file path from a UUID without loading the asset into memory)
		std::string scenePath = assetManager.GetAsset<Scene>(sceneUUID)->GetFilePath();
		if (scenePath.empty())
		{
			EB_CORE_ERROR("Attempted to load invalid Scene UUID: {}", sceneUUID);
			return;
		}

		LoadScene(scenePath);
	}

	void SceneManager::ExecuteSceneSwap()
	{
		if (!m_LoadRequested)
			return;

		EB_CORE_INFO("Swapping to new scene: {}", m_NextScenePath);

		// Fetch the scene from the asset manager — this is the single source of truth.
		// Load<Scene> de-duplicates by path, so if the scene was already registered via
		// CreateScene it returns that same instance rather than creating a second one.
		auto& assetManager = Application::Instance().GetAssetManager();
		auto newScene = assetManager.Load<Scene>(m_NextScenePath, false);

		// Deserialize it using your existing system
		SceneSerializer serializer(newScene);
		if (serializer.Deserialize(m_NextScenePath))
		{
			// If we are currently running a game, make sure the new scene starts running too!
			bool wasRunning = m_ActiveScene && m_ActiveScene->IsRuntime();

			if (m_ActiveScene && wasRunning)
				m_ActiveScene->OnRuntimeStop();

				SetActiveScene(newScene);

				if (wasRunning)
					m_ActiveScene->OnRuntimeStart();

				if (m_OnSceneChanged)
					m_OnSceneChanged(newScene);
			}
			else
			{
				EB_CORE_ERROR("Failed to load scene during transition: {}", m_NextScenePath);
			}

			m_LoadRequested = false;
			m_NextScenePath = "";
	}
}