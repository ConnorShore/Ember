#include "ebpch.h"
#include "ScriptBindMath.h"

#include "Ember/Math/Math.h"

namespace Ember {

	void BindMath(sol::state& state)
	{
		state.new_usertype<Vector3f>("Vector3f",
			sol::constructors<Vector3f(), Vector3f(float, float, float)>(),
			"x", &Vector3f::x,
			"y", &Vector3f::y,
			"z", &Vector3f::z
		);
	}

}
