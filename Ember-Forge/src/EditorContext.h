#pragma once

#include "EditorPreferences.h"
#include "Viewers/EditorViewportViewer.h"

#include <Ember/Core/Application.h>
#include <Ember/Scene/Scene.h>
#include <Ember/Scene/Entity.h>
#include <Ember/Tools/EditorCamera.h>
#include <Ember/Event/Event.h>
#include <Ember/ECS/Types.h>

#include <functional>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>

namespace Ember {

	// Shared state passed to all editor panels and component UIs
	struct EditorContext
	{
		EditorCamera* EditorCamera;

		// Owned by EditorLayer; handed out here so panels and the gizmo controller can read snap and
		// placement settings without reaching back into the layer.
		EditorPreferences* Preferences = nullptr;

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

		// Supplied by EditorLayer, which owns the viewport and camera needed to resolve it.
		std::function<Vector3f()> SpawnPosition;

		void EventCallback(Event& e)
		{
			Application::Instance().OnEvent(e);
		}
	};
}