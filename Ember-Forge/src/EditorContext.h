#pragma once

#include <Ember.h>
#include <unordered_set>

namespace Ember {

	struct EditorContext
	{
		SharedPtr<Scene> ActiveScene;
		Entity SelectedEntity;
		std::unordered_set<Entity> PendingEntityRemovals;
	};
}