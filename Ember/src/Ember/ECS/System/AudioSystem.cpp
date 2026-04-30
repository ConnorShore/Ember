#include "ebpch.h"
#include "AudioSystem.h"

namespace Ember {

	AudioSystem::AudioSystem()
		: m_AudioEngine(ScopedPtr<ma_engine>::Create())
	{
	}

	AudioSystem::~AudioSystem()
	{
	}

	void AudioSystem::OnAttach()
	{
		ma_result result;
		result = ma_engine_init(NULL, m_AudioEngine.Ptr());
		if (result != MA_SUCCESS) {
			EB_CORE_ERROR("Failed to initialize audio engine.");
			return;
		}
	}

	void AudioSystem::OnSceneAttach(Scene* scene)
	{
		ma_engine_play_sound(m_AudioEngine.Ptr(), "C:\\Development\\Projects\\Ember\\Ember\\assets\\audio\\test-sound.wav", NULL);
	}

	void AudioSystem::OnSceneDetach(Scene* scene)
	{
		ma_engine_stop(m_AudioEngine.Ptr());
	}

	void AudioSystem::OnDetach()
	{
		ma_engine_uninit(m_AudioEngine.Ptr());
	}

	void AudioSystem::OnUpdate(TimeStep delta, Scene* scene)
	{

	}

}