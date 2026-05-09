#pragma once

#include "Ember/Core/Core.h"
#include <functional>
#include <string>

namespace Ember {

	class Scene;

	class SceneManager
	{
	public:
		SceneManager() = default;
		~SceneManager() = default;

		SharedPtr<Scene> CreateScene(const std::string& name = "");

		// Returns the currently active scene
		SharedPtr<Scene> GetActiveScene();

		// Immediately sets the scene (used heavily by the Editor)
		void SetActiveScene(SharedPtr<Scene> scene);

		// Queues up a scene to be loaded from disk at the end of the frame
		void LoadScene(UUID sceneUUID);
		void LoadScene(const std::string& filepath);

		// Called by Application::Run() at the very end of the frame
		void ExecuteSceneSwap();

		// Optional callback invoked whenever the active scene changes (e.g. after a deferred swap)
		void SetOnSceneChangedCallback(std::function<void(SharedPtr<Scene>)> callback) { m_OnSceneChanged = std::move(callback); }

	private:
		SharedPtr<Scene> m_ActiveScene;

		// Deferred loading variables
		std::string m_NextScenePath = "";
		bool m_LoadRequested = false;

		std::function<void(SharedPtr<Scene>)> m_OnSceneChanged;
	};
}