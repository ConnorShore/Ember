#pragma once

#include "AudioSoundProperties.h"

#include "Ember/Math/Math.h"
#include "Ember/Asset/AudioClip.h"

#include <string>
#include <miniaudio.h>

namespace Ember {

	class AudioSource
	{
	public:
		AudioSource() = default;
		~AudioSource();

		// Initialization
		bool Load(const std::string& filePath);
		void Unload();

		// Playback Controls
		void Play(AudioSoundProperties& props);
		void Stop();
		void Restart();

		std::atomic<bool> IsPlaying{ false };
		std::atomic<bool> IsQueued{ false };

		AudioSource(const AudioSource& other)
			: IsPlaying(other.IsPlaying.load()), IsQueued(other.IsQueued.load()) {}

		// Need equals overload since copying an atomic bool is a deleted function/constructor
		AudioSource& operator=(const AudioSource& other)
		{
			if (this != &other)
			{
				IsPlaying.store(other.IsPlaying.load());
				IsQueued.store(other.IsQueued.load());
			}
			return *this;
		}

		inline ma_sound* GetSound() { return &m_Sound; }
		inline bool IsLoaded() const { return m_IsLoaded; }
		
	private:
		// Must be static for C-style callback
		static void OnSoundEnd(void* pUserData, ma_sound* pSound);

	private:
		ma_sound m_Sound;
		bool m_IsLoaded = false;
	};
}