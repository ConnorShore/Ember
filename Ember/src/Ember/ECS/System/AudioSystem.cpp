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

	void AudioSystem::OnDetach()
	{
		if (m_AudioEngine != nullptr)
			ma_engine_uninit(m_AudioEngine.Ptr());
	}

	void AudioSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		auto view = scene->GetRegistry().ActiveQuery<AudioSourceComponent>();
		for (auto entity : view)
		{
			auto& audioComp = scene->GetRegistry().GetComponent<AudioSourceComponent>(entity);
			auto& source = audioComp.Source;

			// Has the user requested to play a sound that isn't currently playing?
			if (source.IsQueued && !source.IsPlaying)
			{
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

}