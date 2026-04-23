#include "ebpch.h"
#include "ScriptBindComponents.h"

namespace Ember {
	void BindAllComponents(sol::state& state)
	{
		BindCoreComponents(state);
		BindPhysicsComponents(state);
		BindRenderingComponents(state);
		BindLightingAndCameraComponents(state);
		BindMiscComponents(state);
	}
}
