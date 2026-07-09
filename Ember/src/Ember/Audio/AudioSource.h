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

		AudioSource(const AudioSource&) = delete;

		// Move Constructor
		// This tells the ECS how to safely transfer ownership during an array resize
		AudioSource(AudioSource&& other) noexcept
		{
			// Steal the pointer and state from the old component
			m_Sound = other.m_Sound;
			m_IsLoaded = other.m_IsLoaded;
			IsPlaying.store(other.IsPlaying.load());
			IsQueued.store(other.IsQueued.load());

			// Nullify the old component so its destructor does absolutely nothing
			other.m_Sound = nullptr;
			other.m_IsLoaded = false;
			other.IsPlaying = false;
			other.IsQueued = false;

			// Set the end callback for the new sound if it's valid
			if (m_Sound != nullptr) {
				ma_sound_set_end_callback(m_Sound, OnSoundEnd, this);
			}
		}

		// Initialization
		bool Load(const std::string& filePath);
		void Unload();

		// Playback Controls
		void Play();
		void PlayDelayed(float delayMs);
		void Stop();
		void Restart();

		std::atomic<bool> IsPlaying{ false };
		std::atomic<bool> IsQueued{ false };
		std::atomic<float> DelayMS{ 0.0f };

		// Remove copy semantics
		AudioSource& operator=(const AudioSource&) = delete;

		// The Move Assignment Operator
		AudioSource& operator=(AudioSource&& other) noexcept
		{
			if (this != &other)
			{
				Unload(); // Clean up our current sound before taking the new one

				m_Sound = other.m_Sound;
				m_IsLoaded = other.m_IsLoaded;
				IsPlaying.store(other.IsPlaying.load());
				IsQueued.store(other.IsQueued.load());

				other.m_Sound = nullptr;
				other.m_IsLoaded = false;
				other.IsPlaying = false;
				other.IsQueued = false;

				if (m_Sound != nullptr) {
					ma_sound_set_end_callback(m_Sound, OnSoundEnd, this);
				}
			}
			return *this;
		}

		inline ma_sound* GetSound() { return m_Sound; }
		inline bool IsLoaded() const { return m_IsLoaded; }
		
	private:
		// Must be static for C-style callback
		static void OnSoundEnd(void* pUserData, ma_sound* pSound);

	private:
		ma_sound* m_Sound = nullptr;
		bool m_IsLoaded = false;
	};
}