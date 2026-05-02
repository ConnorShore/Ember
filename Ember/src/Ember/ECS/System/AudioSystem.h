#pragma once

#include "System.h"

#include "Ember/Math/Math.h"

#include "miniaudio.h"

namespace Ember {

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

		void PlaySound(Scene* scene, const std::string& soundName, const Vector3f& position);

		ma_engine* GetAudioEngine() const { return m_AudioEngine.Ptr(); }

	private:
		void RestartEngine();

	private:
		ScopedPtr<ma_engine> m_AudioEngine;
	};

}