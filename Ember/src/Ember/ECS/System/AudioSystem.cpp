#include "ebpch.h"
#include "AudioSystem.h"

#include "Ember/Scene/Scene.h"
#include "Ember/ECS/Component/Components.h"

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
		// Use normal query to load all audio sources in the scene, not just active ones, since we want them to be ready to play immediately when activated
		auto view = scene->GetRegistry().Query<AudioSourceComponent>();
		for (auto entity : view)
		{
			auto& audioComp = scene->GetRegistry().GetComponent<AudioSourceComponent>(entity);
			if (!audioComp.Source.IsLoaded())
			{
				std::string filePath = Application::Instance().GetAssetManager().GetAsset<AudioClip>(audioComp.AudioClipHandle)->GetFilePath();
				audioComp.Source.Load(filePath);
			}
		}
	}

	void AudioSystem::OnSceneDetach(Scene* scene)
	{
		auto view = scene->GetRegistry().Query<AudioSourceComponent>();
		for (auto entity : view)
		{
			auto& audioComp = scene->GetRegistry().GetComponent<AudioSourceComponent>(entity);
			audioComp.Source.Unload();
		}

		// Stop all active one shot sounds
		StopAllOneShotSounds();
	}

	void AudioSystem::OnDetach()
	{
		if (m_AudioEngine != nullptr)
			ma_engine_uninit(m_AudioEngine.Ptr());
	}

	void AudioSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		UpdateAudioSources(scene, delta);
		UpdateAudioListeners(scene);
		CleanupFinishedOneShotSounds();
	}

	void AudioSystem::PlayOneShot(UUID clipHandle, AudioSoundProperties& props, const Vector3f& position /* = Vector3f(0.0) */)
	{
		if (clipHandle == Constants::InvalidUUID)
			return;

		auto& assetManager = Application::Instance().GetAssetManager();
		auto clipAsset = assetManager.GetAsset<AudioClip>(clipHandle);

		if (!clipAsset)
		{
			EB_CORE_WARN("AudioSystem::PlayOneShot failed. Invalid AudioClip handle.");
			return;
		}

		ma_sound* oneShotSound = new ma_sound();
		std::string filePath = clipAsset->GetFilePath();

		// Use MA_SOUND_FLAG_DECODE to force miniaudio to read the file into RAM 
		// instead of streaming it from your hard drive frame-by-frame.
		ma_uint32 flags = MA_SOUND_FLAG_DECODE;

		ma_result result = ma_sound_init_from_file(m_AudioEngine.Ptr(), filePath.c_str(), flags, NULL, NULL, oneShotSound);
		if (result != MA_SUCCESS)
		{
			EB_CORE_ERROR("PlayOneShot failed to load sound: {0}", filePath);
			delete oneShotSound;
			return;
		}

		// Configure the channel for a 2D One-Shot
		ma_sound_set_volume(oneShotSound, props.Volume);
		ma_sound_set_pitch(oneShotSound, props.Pitch);

		if (props.Spatialized)
		{
			ma_sound_set_spatialization_enabled(oneShotSound, MA_TRUE);
			ma_sound_set_position(oneShotSound, position.x, position.y, position.z);
			ma_sound_set_min_distance(oneShotSound, props.MinDistance);
			ma_sound_set_max_distance(oneShotSound, props.MaxDistance);
		}
		else
		{
			ma_sound_set_spatialization_enabled(oneShotSound, MA_FALSE);
			ma_sound_set_looping(oneShotSound, MA_FALSE);
		}

		m_OneShotSounds.push_back(oneShotSound);
		ma_sound_start(oneShotSound);
	}

	void AudioSystem::PlaySound(Scene* scene, const std::string& soundName, AudioSoundProperties& props, const Vector3f& position /* = Vector3f(0.0f) */)
	{
		PlaySoundDelayed(scene, soundName, 0.0f, props, position);
	}

	void AudioSystem::PlaySoundDelayed(Scene* scene, const std::string& soundName, float delayMs, AudioSoundProperties, const Vector3f& position /*= Vector3f(0.0f)*/)
	{
		Entity emitter = scene->AddEntity("SoundEmitter");
		emitter.GetComponent<TransformComponent>().Position = position;

		// Attach and load the audio
		auto filepath = Application::Instance().GetAssetManager().GetAsset<AudioClip>(soundName)->GetFilePath();
		auto& audioComp = emitter.AttachComponent<AudioSourceComponent>();
		audioComp.Source.Load(filepath);

		// Tag it for auto-destruction and tell it to play
		emitter.AttachComponent<SingleSoundComponent>();
		audioComp.Source.PlayDelayed(delayMs);
	}
	
	void AudioSystem::RestartEngine()
	{
		if (m_AudioEngine != nullptr) 
		{
			ma_engine_stop(m_AudioEngine.Ptr());
			ma_engine_uninit(m_AudioEngine.Ptr());
			m_AudioEngine = nullptr;
		}

		ma_result result;
		result = ma_engine_init(NULL, m_AudioEngine.Ptr());
		if (result != MA_SUCCESS) {
			EB_CORE_ERROR("Failed to initialize audio engine.");
			return;
		}
	}

	void AudioSystem::UpdateAudioSources(Scene* scene, TimeStep delta)
	{
		auto view = scene->GetRegistry().ActiveQuery<AudioSourceComponent>();
		for (auto entityId : view)
		{
			Entity entity(entityId, scene);

			auto& audioComp = entity.GetComponent<AudioSourceComponent>();
			auto& source = audioComp.Source;
			auto& transform = entity.GetComponent<TransformComponent>();

			// Check if single sound source
			if (entity.ContainsComponent<SingleSoundComponent>())
			{
				// Remove finished sounds
				if (!audioComp.Source.IsPlaying && !audioComp.Source.IsQueued)
				{
					scene->RemoveEntity(entity);
					continue;
				}
			}

			// Set sound location if it's spatialized
			if (audioComp.Properties.Spatialized)
			{
				auto& transform = entity.GetComponent<TransformComponent>();
				ma_sound_set_position(source.GetSound(), transform.Position.x, transform.Position.y, transform.Position.z);
			}

			if (!source.IsPlaying)
			{
				if (audioComp.PlayOnStart)
				{
					// Start sound if played on start and not yet playing
					StartSound(audioComp, transform.Position);
					source.IsPlaying = true;
				}
				else if (source.IsQueued)
				{
					// Start sound if queue'd and not yet playing
					source.DelayMS -= delta.Milliseconds();
					if (source.DelayMS <= 0.0f)
					{
						StartSound(audioComp, transform.Position);

						// Update our state machine
						source.IsPlaying = true;
						source.IsQueued = false;
						source.DelayMS = 0.0f;
					}
				}
			}
		}
	}

	void AudioSystem::UpdateAudioListeners(Scene* scene)
	{
		auto listenerView = scene->GetRegistry().ActiveQuery<AudioListenerComponent, TransformComponent>();
		for (auto entity : listenerView)
		{
			auto& listener = scene->GetRegistry().GetComponent<AudioListenerComponent>(entity);
			if (!listener.IsActive)
				continue;

			auto& transform = scene->GetRegistry().GetComponent<TransformComponent>(entity);
			Vector3f pos = transform.Position;
			Vector3f forward = transform.GetForward();
			Vector3f up = transform.GetUp();

			// Pass the specific ListenerIndex into miniaudio
			ma_engine_listener_set_position(m_AudioEngine.Ptr(), listener.ListenerIndex, pos.x, pos.y, pos.z);
			ma_engine_listener_set_direction(m_AudioEngine.Ptr(), listener.ListenerIndex, forward.x, forward.y, forward.z);
			ma_engine_listener_set_world_up(m_AudioEngine.Ptr(), listener.ListenerIndex, up.x, up.y, up.z);
		}
	}

	void AudioSystem::CleanupFinishedOneShotSounds()
	{
		// Clean up finished one-shot sounds safely on the main thread
		for (auto it = m_OneShotSounds.begin(); it != m_OneShotSounds.end(); )
		{
			ma_sound* sound = *it;

			// Check if the sound has reached the end of its PCM data
			if (ma_sound_at_end(sound))
			{
				// Safe to uninitialize and delete here!
				ma_sound_uninit(sound);
				delete sound;

				// Remove from the vector and advance the iterator
				it = m_OneShotSounds.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void AudioSystem::StopAllOneShotSounds()
	{
		for (auto* sound : m_OneShotSounds)
		{
			ma_sound_stop(sound);
			ma_sound_uninit(sound);
			delete sound;
		}
		m_OneShotSounds.clear();
	}

	void AudioSystem::StartSound(AudioSourceComponent& sourceComp, const Vector3f& position)
	{
		auto& source = sourceComp.Source;

		// Set spatial properties
		if (sourceComp.Properties.Spatialized)
		{
			// Explicitly enable spatialization (or disable it if they unchecked the box!)
			ma_sound_set_spatialization_enabled(source.GetSound(), MA_TRUE);

			// Set 3D position of sound
			ma_sound_set_position(source.GetSound(), position.x, position.y, position.z);

			// Set min and max distance
			ma_sound_set_min_distance(source.GetSound(), sourceComp.Properties.MinDistance);
			ma_sound_set_max_distance(source.GetSound(), sourceComp.Properties.MaxDistance);
		}
		else
		{
			ma_sound_set_spatialization_enabled(source.GetSound(), MA_FALSE);
		}

		// Set looping
		if (sourceComp.Properties.Looping)
			ma_sound_set_looping(source.GetSound(), MA_TRUE);

		// Set volume and pitch
		ma_sound_set_volume(source.GetSound(), sourceComp.Properties.Volume);
		ma_sound_set_pitch(source.GetSound(), sourceComp.Properties.Pitch);

		// Rewind the sound to the beginning just in case it was played previously
		ma_sound_seek_to_pcm_frame(source.GetSound(), 0);

		// Tell the audio thread to start
		ma_sound_start(source.GetSound());
	}
}