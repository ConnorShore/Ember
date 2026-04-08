#pragma once
#include "Ember/Core/Core.h"
#include "Ember/Asset/Animation.h"

namespace Ember {

	class AnimationSerializer
	{
	public:
		static bool Serialize(const std::filesystem::path& filepath, const SharedPtr<Animation>& animation);
		static SharedPtr<Animation> Deserialize(UUID uuid, const std::filesystem::path& filepath);
	};

}