#pragma once

#include "System.h"

#include "Ember/Math/Math.h"
#include "Ember/Audio/AudioSoundProperties.h"

#include "miniaudio.h"

namespace Ember {

	struct AudioSourceComponent;

	class AudioSystem : public System
	{
	public:
		AudioSystem();
		virtual ~AudioSystem();

		void OnAttach() override;
		void OnSceneAttach(Scene* scene) override;
		void OnSceneDetach(Scene* scene) override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;

		void PlaySound(Scene* scene, const std::string& soundName, AudioSoundProperties& props, const Vector3f& position = Vector3f(0.0f));

		ma_engine* GetAudioEngine() const { return m_AudioEngine.Ptr(); }

	private:
		void RestartEngine();
		void UpdateAudioSources(Scene* scene);
		void UpdateAudioListeners(Scene* scene);
		void StartSound(AudioSourceComponent& sourceComp, const Vector3f& position);

	private:
		ScopedPtr<ma_engine> m_AudioEngine;
	};

}