#pragma once

#include <sol/sol.hpp>

namespace Ember {

	class Scene;

	void BindScene(sol::state& state, Scene* scene);

}
