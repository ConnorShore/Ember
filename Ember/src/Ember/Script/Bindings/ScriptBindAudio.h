#pragma once

#include <sol/sol.hpp>

namespace Ember {

	class Scene;

	void BindAudio(sol::state& state, Scene* scene);

}
