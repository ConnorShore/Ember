#pragma once

#include "Ember/Scene/Scene.h"

#include <sol/sol.hpp>

namespace Ember {

	void BindPhysics(sol::state& state, Scene* scene);

}
