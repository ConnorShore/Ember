#pragma once

#include "Asset.h"

namespace Ember {

	class AudioClip : public Asset
	{
	public:
		AudioClip(const std::string& name, const std::string& filePath);
		AudioClip(UUID uuid, const std::string& name, const std::string& filePath);

		virtual ~AudioClip() = default;

		static AssetType GetStaticType() { return AssetType::AudioClip; }

	private:

	};

}