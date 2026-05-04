#include "ebpch.h"
#include "AudioClip.h"

namespace Ember {

	AudioClip::AudioClip(const std::string& name, const std::string& filePath)
		: AudioClip(UUID(), name, filePath)
	{
	}

	AudioClip::AudioClip(UUID uuid, const std::string& name, const std::string& filePath)
		: Asset(uuid, name, filePath, AssetType::AudioClip)
	{
	}

}