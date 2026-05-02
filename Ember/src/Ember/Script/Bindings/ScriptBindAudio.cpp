#include "ebpch.h"
#include "ScriptBindAudio.h"

#include "Ember/Scene/Scene.h"
#include "Ember/ECS/System/AudioSystem.h"

namespace Ember {

	void BindAudio(sol::state& state, Scene* scene)
	{
		sol::table audioSystem = state.create_named_table("AudioSystem");
		audioSystem.set_function("PlaySound", [&state, scene](const std::string& soundAssetName, const Vector3f& position) {
			auto audioSystem = Application::Instance().GetSystem<AudioSystem>();
			audioSystem->PlaySound(scene, soundAssetName, position);
		});
	}

}
