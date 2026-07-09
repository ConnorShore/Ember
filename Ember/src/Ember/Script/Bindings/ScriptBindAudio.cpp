#include "ebpch.h"
#include "ScriptBindAudio.h"

#include "Ember/Scene/Scene.h"
#include "Ember/ECS/System/AudioSystem.h"

namespace Ember {

	static void PlaySound(Scene* scene, const std::string& soundAssetName, float delayMs, AudioSoundProperties& props, const Vector3f& position)
	{
		auto audioSystem = Application::Instance().GetSystem<AudioSystem>();
		audioSystem->PlaySoundDelayed(scene, soundAssetName, delayMs, props, position);
	}

	static void PlaySound(Scene* scene, UUID soundAssetUUID, float delayMs, AudioSoundProperties& props, const Vector3f& position)
	{
		auto& assetManager = Application::Instance().GetAssetManager();
		if (soundAssetUUID == Constants::InvalidUUID || !assetManager.ContainsAsset(soundAssetUUID))
			return;
		auto clip = assetManager.GetAsset<AudioClip>(soundAssetUUID);
		if (!clip)
			return;

		PlaySound(scene, clip->GetName(), delayMs, props, position);
	}

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
				AudioSoundProperties props;
				Vector3f position(0.0f);
				PlaySound(scene, soundAssetName, 0.0f, props, position);
			},
			[scene](const std::string& soundAssetName, AudioSoundProperties& props) {
				Vector3f position(0.0f);
				PlaySound(scene, soundAssetName, 0.0f, props, position);
			},
			[scene](const std::string& soundAssetName, AudioSoundProperties& props, const Vector3f& position) {
				PlaySound(scene, soundAssetName, 0.0f, props, position);
			},
			[scene](UUID soundAssetUUID) {
				AudioSoundProperties props;
				Vector3f position(0.0f);
				PlaySound(scene, soundAssetUUID, 0.0f, props, position);
			},
			[scene](UUID soundAssetUUID, AudioSoundProperties& props) {
				Vector3f position(0.0f);
				PlaySound(scene, soundAssetUUID, 0.0f, props, position);
			},
			[scene](UUID soundAssetUUID, AudioSoundProperties& props, const Vector3f& position) {
				PlaySound(scene, soundAssetUUID, 0.0f, props, position);
			}
		));
		audioSystem.set_function("PlaySoundDelayed", sol::overload(
			[scene](const std::string& soundAssetName, float delayMs) {
				AudioSoundProperties props;
				Vector3f position(0.0f);
				PlaySound(scene, soundAssetName, delayMs, props, position);
			},
			[scene](const std::string& soundAssetName, float delayMs, AudioSoundProperties& props) {
				Vector3f position(0.0f);
				PlaySound(scene, soundAssetName, delayMs, props, position);
			},
			[scene](const std::string& soundAssetName, float delayMs, AudioSoundProperties& props, const Vector3f& position) {
				PlaySound(scene, soundAssetName, delayMs, props, position);
			},
			[scene](UUID soundAssetUUID, float delayMs) {
				AudioSoundProperties props;
				Vector3f position(0.0f);
				PlaySound(scene, soundAssetUUID, delayMs, props, position);
			},
			[scene](UUID soundAssetUUID, float delayMs, AudioSoundProperties& props) {
				Vector3f position(0.0f);
				PlaySound(scene, soundAssetUUID, delayMs, props, position);
			},
			[scene](UUID soundAssetUUID, float delayMs, AudioSoundProperties& props, const Vector3f& position) {
				PlaySound(scene, soundAssetUUID, delayMs, props, position);
			}
		));

		audioSystem.set_function("PlayOneShot", sol::overload(
			[](UUID clipHandle) {
				auto audioSystem = Application::Instance().GetSystem<AudioSystem>();
				AudioSoundProperties props;
				audioSystem->PlayOneShot(clipHandle, props);
			},
			[](UUID clipHandle, AudioSoundProperties& props) {
				auto audioSystem = Application::Instance().GetSystem<AudioSystem>();
				audioSystem->PlayOneShot(clipHandle, props);
			},
			[](UUID clipHandle, AudioSoundProperties& props, const Vector3f& position) {
				auto audioSystem = Application::Instance().GetSystem<AudioSystem>();
				audioSystem->PlayOneShot(clipHandle, props, position);
			}
		));
	}

}
