#pragma once

#include "Ember/Scene/Scene.h"

#include <sol/sol.hpp>

namespace Ember {
	// Individual domain binders. ScriptEngine calls these directly rather than through one
	// bind-everything helper: all of them except BindAIComponents are scene-independent and must be
	// registered only once per Lua state (see s_StatelessBindingsRegistered in ScriptEngine.cpp).
	void BindCoreComponents(sol::state& state);
	void BindPhysicsComponents(sol::state& state);
	void BindRenderingComponents(sol::state& state);
	void BindLightingAndCameraComponents(sol::state& state);
	void BindAudioComponents(sol::state& state);
	void BindAIComponents(sol::state& state, Scene* scene);
	void BindMiscComponents(sol::state& state);
}