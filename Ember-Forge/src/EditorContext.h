#pragma once

#include "EditorPreferences.h"
#include "Viewers/EditorViewportViewer.h"

#include <Ember/Core/Application.h>
#include <Ember/Scene/Scene.h>
#include <Ember/Scene/Entity.h>
#include <Ember/Tools/EditorCamera.h>
#include <Ember/Event/Event.h>
#include <Ember/ECS/Types.h>

#include <algorithm>
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

		// The active entity: the last one clicked, and the only one the inspector edits. Kept as a
		// mirror of SelectedEntities.back() so every panel that wants just one entity is unchanged.
		Entity SelectedEntity;

		// The whole selection, in click order. Only the gizmo, outlining, delete and duplicate paths
		// walk this; everything else reads SelectedEntity.
		std::vector<Entity> SelectedEntities;

		bool IsSelected(Entity entity) const
		{
			return std::find(SelectedEntities.begin(), SelectedEntities.end(), entity) != SelectedEntities.end();
		}

		size_t SelectionCount() const { return SelectedEntities.size(); }

		// Public because EditorContext must stay an aggregate for its designated-initialiser setup.
		void SyncActiveEntity()
		{
			SelectedEntity = SelectedEntities.empty() ? Entity() : SelectedEntities.back();
		}

		void ClearSelection()
		{
			SelectedEntities.clear();
			SyncActiveEntity();
		}

		void SetSelection(Entity entity)
		{
			SelectedEntities.clear();
			if (entity.IsValid())
				SelectedEntities.push_back(entity);
			SyncActiveEntity();
		}

		void SetSelection(const std::vector<Entity>& entities)
		{
			SelectedEntities.clear();
			for (Entity entity : entities)
			{
				if (entity.IsValid() && !IsSelected(entity))
					SelectedEntities.push_back(entity);
			}
			SyncActiveEntity();
		}

		void AddToSelection(Entity entity)
		{
			if (!entity.IsValid() || IsSelected(entity))
				return;

			SelectedEntities.push_back(entity);
			SyncActiveEntity();
		}

		void RemoveFromSelection(Entity entity)
		{
			std::erase(SelectedEntities, entity);
			SyncActiveEntity();
		}

		// Ctrl+click behaviour: pull an already-selected entity out, otherwise add it.
		void ToggleSelection(Entity entity)
		{
			if (!entity.IsValid())
				return;

			if (IsSelected(entity))
				RemoveFromSelection(entity);
			else
				AddToSelection(entity);
		}

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