#pragma once

#include "Ember/Scene/Scene.h"

#include <sol/sol.hpp>

namespace Ember {
	// Individual domain binders
	void BindCoreComponents(sol::state& state);
	void BindPhysicsComponents(sol::state& state);
	void BindRenderingComponents(sol::state& state);
	void BindLightingAndCameraComponents(sol::state& state);
	void BindAudioComponents(sol::state& state);
	void BindAIComponents(sol::state& state, Scene* scene);
	void BindMiscComponents(sol::state& state);

	// Master caller
	void BindAllComponents(sol::state& state, Scene* scene);
}