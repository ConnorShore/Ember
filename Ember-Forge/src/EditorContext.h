#pragma once

#include "Viewers/EditorViewportViewer.h"

#include <Ember/Core/Application.h>
#include <Ember/Scene/Scene.h>
#include <Ember/Scene/Entity.h>
#include <Ember/Tools/EditorCamera.h>
#include <Ember/Event/Event.h>
#include <Ember/ECS/Types.h>

#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>

namespace Ember {

	// Shared state passed to all editor panels and component UIs
	struct EditorContext
	{
		EditorCamera* EditorCamera;

		Entity SelectedEntity;

		// Returns the currently active scene from the SceneManager (single source of truth)
		SharedPtr<Scene> ActiveScene() const { return Application::Instance().GetSceneManager().GetActiveScene(); }

		SceneState CurrentSceneState = SceneState::Edit;
		EditorViewportViewer* ActiveViewportViewer = nullptr;

		bool IsEditingPrefab = false;
		Entity PrefabRootEntity;
		std::string ActivePrefabPath;
		std::string RequestedSceneOpenPath;
		std::string RequestedPrefabOpenPath;
		std::string RequestAnimationStateOpenPath;
		std::string RequestSkeletonMaskOpenPath;

		// Deferred removals: entities/components are queued during rendering and
		// actually removed after the frame to avoid invalidating iterators.
		std::unordered_set<Entity> PendingEntityRemovals;
		std::unordered_map<Entity, std::vector<ComponentType>> PendingComponentRemovals;

		void EventCallback(Event& e)
		{
			Application::Instance().OnEvent(e);
		}
	};
}