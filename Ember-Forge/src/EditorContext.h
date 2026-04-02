#pragma once

#include <Ember.h>

#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace Ember {

	struct EditorContext
	{
		SharedPtr<Scene> ActiveScene;
		EditorCamera* EditorCamera;
		Entity SelectedEntity;

		std::unordered_set<Entity> PendingEntityRemovals;
		std::unordered_map<Entity, std::vector<ComponentType>> PendingComponentRemovals;

		void EventCallback(Event& e)
		{
			Application::Instance().OnEvent(e);
		}
	};
}