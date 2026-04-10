#pragma once

#include <Ember/Core/Application.h>
#include <Ember/Scene/Scene.h>
#include <Ember/Scene/Entity.h>
#include <Ember/Tools/EditorCamera.h>
#include <Ember/Event/Event.h>
#include <Ember/ECS/Types.h>

#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace Ember {

	// Shared state passed to all editor panels and component UIs
	struct EditorContext
	{
		SharedPtr<Scene> ActiveScene;
		EditorCamera* EditorCamera;
		Entity SelectedEntity;

		SceneState CurrentSceneState = SceneState::Edit;

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