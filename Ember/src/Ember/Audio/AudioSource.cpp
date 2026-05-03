#include "ebpch.h"
#include "AudioSource.h"

#include "Ember/Core/Application.h"
#include "Ember/ECS/System/AudioSystem.h"

namespace Ember {

	AudioSource::~AudioSource()
	{
		Unload();
	}

	bool AudioSource::Load(const std::string& filePath)
	{
		auto audioSystem = Application::Instance().GetSystemManager().GetSystem<AudioSystem>();

		m_Sound = new ma_sound();
		ma_result result = ma_sound_init_from_file(audioSystem->GetAudioEngine(), filePath.c_str(), 0, NULL, NULL, m_Sound);
		if (result != MA_SUCCESS)
		{
			EB_CORE_ERROR("Failed to load sound from file: {0}", filePath);
			delete m_Sound;
			m_Sound = nullptr;
			return false;
		}

		// Register callback for when sound finishes playing
		ma_sound_set_end_callback(m_Sound, OnSoundEnd, this);

		m_IsLoaded = true;
		return true;
	}

	void AudioSource::Unload()
	{
		if (m_IsLoaded && m_Sound != nullptr)
		{
			ma_sound_stop(m_Sound);
			ma_sound_set_end_callback(m_Sound, nullptr, nullptr);
			ma_sound_uninit(m_Sound);

			delete m_Sound;
			m_Sound = nullptr;

			m_IsLoaded = false;
			IsPlaying = false;
			IsQueued = false;
		}
	}

	void AudioSource::Play(AudioSoundProperties& props)
	{
		IsQueued = true;
	}

	void AudioSource::Stop()
	{
		ma_sound_stop(m_Sound);
		IsQueued = false;
		IsPlaying = false;
	}

	void AudioSource::Restart()
	{
		// TODO: Need to set trigger restart flag
	}

	void AudioSource::OnSoundEnd(void* pUserData, ma_sound* pSound)
	{
		AudioSource* source = static_cast<AudioSource*>(pUserData);
		source->IsPlaying = false;
		source->IsQueued = false;
	}

}