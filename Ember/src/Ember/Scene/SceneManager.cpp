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

		auto newScene = assetManager.Create<Scene>(sceneName, fullScenePath.string());
		SceneSerializer serializer(newScene);
		if (!serializer.Serialize(fullScenePath.string()))
		{
			return nullptr;
		}

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

		// Create the new scene
		SharedPtr<Scene> newScene = SharedPtr<Scene>::Create("Loaded Scene", m_NextScenePath);
		newScene->SetFilePath(m_NextScenePath);

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