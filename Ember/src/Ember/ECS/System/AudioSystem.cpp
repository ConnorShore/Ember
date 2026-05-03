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
	}

	void AudioSystem::OnDetach()
	{
		if (m_AudioEngine != nullptr)
			ma_engine_uninit(m_AudioEngine.Ptr());
	}

	void AudioSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		// Update audio sources
		auto view = scene->GetRegistry().ActiveQuery<AudioSourceComponent>();
		for (auto entityId : view)
		{
			Entity entity(entityId, scene);

			auto& audioComp = entity.GetComponent<AudioSourceComponent>();
			auto& source = audioComp.Source;

			// Check if single sound source
			if (entity.ContainsComponent<SingleSoundComponent>())
			{
				if (!audioComp.Source.IsPlaying && !audioComp.Source.IsQueued)
				{
					scene->RemoveEntity(entity);
					continue;
				}
			}

			// Set sound location if it's spatialized
			if (audioComp.Properties.Spatialized)
			{
				// Explicitly enable spatialization (or disable it if they unchecked the box!)
				ma_sound_set_spatialization_enabled(source.GetSound(), MA_TRUE);

				// Set 3D position of sound
				auto& transform = entity.GetComponent<TransformComponent>();
				ma_sound_set_position(source.GetSound(), transform.Position.x, transform.Position.y, transform.Position.z);

				// Set min and max distance (Ensure MinDistance is > 0.0f !)
				ma_sound_set_min_distance(source.GetSound(), audioComp.Properties.MinDistance);
				ma_sound_set_max_distance(source.GetSound(), audioComp.Properties.MaxDistance);
			}
			else
			{
				// If it's a UI click or background music, turn spatialization OFF
				ma_sound_set_spatialization_enabled(source.GetSound(), MA_FALSE);
			}

			if (source.IsQueued && !source.IsPlaying)
			{
				if (audioComp.Properties.Spatialized)
				{
					// Set 3D position of sound
					auto& transform = entity.GetComponent<TransformComponent>();
					ma_sound_set_position(source.GetSound(), transform.Position.x, transform.Position.y, transform.Position.z);

					// Set min and max distance
					ma_sound_set_min_distance(source.GetSound(), audioComp.Properties.MinDistance);
					ma_sound_set_max_distance(source.GetSound(), audioComp.Properties.MaxDistance);
				}

				// Set volume and pitch
				ma_sound_set_volume(source.GetSound(), audioComp.Properties.Volume);
				ma_sound_set_pitch(source.GetSound(), audioComp.Properties.Pitch);

				// Rewind the sound to the beginning just in case it was played previously
				ma_sound_seek_to_pcm_frame(source.GetSound(), 0);

				// Tell the audio thread to start
				ma_sound_start(source.GetSound());

				// Update our state machine
				source.IsPlaying = true;
				source.IsQueued = false;
			}
		}

		// Update audio listeners
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

	void AudioSystem::PlaySound(Scene* scene, const std::string& soundName, AudioSoundProperties& props, const Vector3f& position /* = Vector3f(0.0f) */)
	{
		Entity emitter = scene->AddEntity("SoundEmitter");
		emitter.GetComponent<TransformComponent>().Position = position;

		// Attach and load the audio
		auto filepath = Application::Instance().GetAssetManager().GetAsset<AudioClip>(soundName)->GetFilePath();
		auto& audioComp = emitter.AttachComponent<AudioSourceComponent>();
		audioComp.Source.Load(filepath);

		// Tag it for auto-destruction and tell it to play
		emitter.AttachComponent<SingleSoundComponent>();
		audioComp.Source.Play(props);
	}

}