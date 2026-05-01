#pragma once

#include "System.h"

#include "miniaudio.h"

namespace Ember {

	class AudioSystem : public System
	{
	public:
		AudioSystem();
		virtual ~AudioSystem();

		void OnAttach() override;
		void OnSceneAttach(Scene* scene) override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;

		ma_engine* GetAudioEngine() const { return m_AudioEngine.Ptr(); }

	private:
		void RestartEngine();

	private:
		ScopedPtr<ma_engine> m_AudioEngine;
	};

}