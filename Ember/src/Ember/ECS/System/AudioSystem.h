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
		void OnSceneDetach(Scene* scene) override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;

	private:
		ScopedPtr<ma_engine> m_AudioEngine;
	};

}