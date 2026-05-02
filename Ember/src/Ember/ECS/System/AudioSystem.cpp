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
		auto view = scene->GetRegistry().ActiveQuery<AudioSourceComponent>();
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
		auto view = scene->GetRegistry().ActiveQuery<AudioSourceComponent>();
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
				}
			}

			if (source.IsQueued && !source.IsPlaying)
			{
				// Rewind the sound to the beginning just in case it was played previously
				ma_sound_seek_to_pcm_frame(source.GetSound(), 0);

				// Tell the audio thread to start
				ma_sound_start(source.GetSound());

				// Update our state machine
				source.IsPlaying = true;
				source.IsQueued = false;
			}
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

	void AudioSystem::PlaySound(Scene* scene, const std::string& soundName, const Vector3f& position)
	{
		Entity emitter = scene->AddEntity("SoundEmitter");
		emitter.GetComponent<TransformComponent>().Position = position;

		// Attach and load the audio
		auto filepath = Application::Instance().GetAssetManager().GetAsset<AudioClip>(soundName)->GetFilePath();
		auto& audioComp = emitter.AttachComponent<AudioSourceComponent>();
		audioComp.Source.Load(filepath);

		// Tag it for auto-destruction and tell it to play
		emitter.AttachComponent<SingleSoundComponent>();

		AudioSoundProperties props;
		//props.Spatialized = true;
		audioComp.Source.Play(props);
	}

}