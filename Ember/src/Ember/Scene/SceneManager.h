#pragma once

#include "Ember/Core/Core.h"
#include <string>

namespace Ember {

	class Scene;

	class SceneManager
	{
	public:
		SceneManager() = default;
		~SceneManager() = default;

		// Returns the currently active scene
		SharedPtr<Scene> GetActiveScene();

		// Immediately sets the scene (used heavily by the Editor)
		void SetActiveScene(SharedPtr<Scene> scene);

		// Queues up a scene to be loaded from disk at the end of the frame
		void LoadScene(const std::string& filepath);

		// Called by Application::Run() at the very end of the frame
		void ExecuteSceneSwap();

	private:
		SharedPtr<Scene> m_ActiveScene;

		// Deferred loading variables
		std::string m_NextScenePath = "";
		bool m_LoadRequested = false;
	};
}