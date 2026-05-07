#include "ebpch.h"
#include "ScriptBindComponents.h"

namespace Ember {
	void BindAllComponents(sol::state& state, Scene* scene)
	{
		BindCoreComponents(state);
		BindPhysicsComponents(state);
		BindRenderingComponents(state);
		BindLightingAndCameraComponents(state);
		BindAudioComponents(state);
		BindAIComponents(state, scene);
		BindMiscComponents(state);
	}
}
