#pragma once

#include "Asset.h"

namespace Ember {

	// Controls how a clip's audio data is brought into memory when a sound is created from it.
	enum class AudioLoadMode : uint8_t
	{
		Auto = 0,	// Pick Decode or Stream based on file size (see AudioSystem::ResolveLoadMode)
		Decode,		// Decode into RAM once and keep it cached. Best for short, frequently played SFX.
		Stream		// Decode on the fly, keeping ~2s buffered. Best for music and other long clips.
	};

	static std::string GetAudioLoadModeString(AudioLoadMode mode)
	{
		switch (mode)
		{
		case AudioLoadMode::Auto: return "Auto";
		case AudioLoadMode::Decode: return "Decode";
		case AudioLoadMode::Stream: return "Stream";
		default: EB_CORE_ASSERT(false, "Unknown audio load mode!"); return "Auto";
		}
	}

	static AudioLoadMode GetAudioLoadModeFromString(const std::string& modeStr)
	{
		if (modeStr == "Auto") return AudioLoadMode::Auto;
		if (modeStr == "Decode") return AudioLoadMode::Decode;
		if (modeStr == "Stream") return AudioLoadMode::Stream;

		EB_CORE_WARN("Unknown audio load mode '{0}'! Falling back to Auto.", modeStr);
		return AudioLoadMode::Auto;
	}

	class AudioClip : public Asset
	{
	public:
		AudioClip(const std::string& name, const std::string& filePath);
		AudioClip(UUID uuid, const std::string& name, const std::string& filePath);

		virtual ~AudioClip() = default;

		static AssetType GetStaticType() { return AssetType::AudioClip; }

		AudioLoadMode GetLoadMode() const { return m_LoadMode; }
		void SetLoadMode(AudioLoadMode mode) { m_LoadMode = mode; }

	private:
		AudioLoadMode m_LoadMode = AudioLoadMode::Auto;
	};

}
