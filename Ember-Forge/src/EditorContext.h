#pragma once

#include <Ember.h>

namespace Ember {

	struct EditorContext
	{
		SharedPtr<Scene> ActiveScene;
		Entity SelectedEntity;
	};
}