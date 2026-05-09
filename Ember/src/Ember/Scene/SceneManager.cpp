#include "ebpch.h"
#include "SceneManager.h"
#include "Scene.h"
#include "SceneSerializer.h"

namespace Ember {

	SharedPtr<Scene> SceneManager::GetActiveScene()
	{
		return m_ActiveScene;
	}

	void SceneManager::SetActiveScene(SharedPtr<Scene> scene)
	{
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

	void SceneManager::ExecuteSceneSwap()
	{
		if (!m_LoadRequested)
			return;

		EB_CORE_INFO("Swapping to new scene: {}", m_NextScenePath);

		// Create the new scene
		SharedPtr<Scene> newScene = SharedPtr<Scene>::Create("Loaded Scene");
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
		}
		else
		{
			EB_CORE_ERROR("Failed to load scene during transition: {}", m_NextScenePath);
		}

		m_LoadRequested = false;
		m_NextScenePath = "";
	}
}