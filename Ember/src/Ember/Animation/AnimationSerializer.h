#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Animation/Animation.h"
#include "Ember/Asset/AssetSerializationMode.h"

namespace Ember {

	class AnimationSerializer
	{
	public:
		static bool SerializeSource(const std::filesystem::path& filepath, const SharedPtr<Animation>& animation);
		static SharedPtr<Animation> DeserializeSource(UUID uuid, const std::filesystem::path& filepath);

		static bool SerializeCooked(const std::filesystem::path& filepath, const SharedPtr<Animation>& animation);
		static SharedPtr<Animation> DeserializeCooked(UUID uuid, const std::filesystem::path& filepath);

		static bool Serialize(const std::filesystem::path& filepath, const SharedPtr<Animation>& animation);
		static SharedPtr<Animation> Deserialize(UUID uuid, const std::filesystem::path& filepath);
	};

}