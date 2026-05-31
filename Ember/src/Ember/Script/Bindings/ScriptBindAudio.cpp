#include "ebpch.h"
#include "ScriptBindAudio.h"

#include "Ember/Scene/Scene.h"
#include "Ember/ECS/System/AudioSystem.h"

namespace Ember {

	void BindAudio(sol::state& state, Scene* scene)
	{
		state.new_usertype<AudioSoundProperties>("AudioSoundProperties",
			sol::constructors<AudioSoundProperties()>(),
			"Volume", &AudioSoundProperties::Volume,
			"Pitch", &AudioSoundProperties::Pitch,
			"Looping", &AudioSoundProperties::Looping,
			"Spatialized", &AudioSoundProperties::Spatialized,
			"MinDistance", &AudioSoundProperties::MinDistance,
			"MaxDistance", &AudioSoundProperties::MaxDistance
		);

		sol::table audioSystem = state.create_named_table("AudioSystem");
		audioSystem.set_function("PlaySound", sol::overload(
			[scene](const std::string& soundAssetName) {
				auto audioSystem = Application::Instance().GetSystem<AudioSystem>();
				AudioSoundProperties props;
				audioSystem->PlaySound(scene, soundAssetName, props);
			},
			[scene](const std::string& soundAssetName, AudioSoundProperties& props) {
				auto audioSystem = Application::Instance().GetSystem<AudioSystem>();
				audioSystem->PlaySound(scene, soundAssetName, props);
			},
			[scene](const std::string& soundAssetName, AudioSoundProperties& props, const Vector3f& position) {

				auto audioSystem = Application::Instance().GetSystem<AudioSystem>();
				audioSystem->PlaySound(scene, soundAssetName, props, position);
			},
			[scene](UUID soundAssetUUID) {
				auto& assetManager = Application::Instance().GetAssetManager();
				if (soundAssetUUID == Constants::InvalidUUID || !assetManager.ContainsAsset(soundAssetUUID))
					return;
				auto clip = assetManager.GetAsset<AudioClip>(soundAssetUUID);
				if (!clip)
					return;

				auto audioSystem = Application::Instance().GetSystem<AudioSystem>();
				AudioSoundProperties props;
				audioSystem->PlaySound(scene, clip->GetName(), props);
			},
			[scene](UUID soundAssetUUID, AudioSoundProperties& props) {
				auto& assetManager = Application::Instance().GetAssetManager();
				if (soundAssetUUID == Constants::InvalidUUID || !assetManager.ContainsAsset(soundAssetUUID))
					return;
				auto clip = assetManager.GetAsset<AudioClip>(soundAssetUUID);
				if (!clip)
					return;

				auto audioSystem = Application::Instance().GetSystem<AudioSystem>();
				audioSystem->PlaySound(scene, clip->GetName(), props);
			},
			[scene](UUID soundAssetUUID, AudioSoundProperties& props, const Vector3f& position) {
				auto& assetManager = Application::Instance().GetAssetManager();
				if (soundAssetUUID == Constants::InvalidUUID || !assetManager.ContainsAsset(soundAssetUUID))
					return;
				auto clip = assetManager.GetAsset<AudioClip>(soundAssetUUID);
				if (!clip)
					return;

				auto audioSystem = Application::Instance().GetSystem<AudioSystem>();
				audioSystem->PlaySound(scene, clip->GetName(), props, position);
			}
		));
	}

}
