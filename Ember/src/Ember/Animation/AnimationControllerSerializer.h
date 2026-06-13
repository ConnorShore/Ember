#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Animation/AnimationController.h"

namespace Ember {

	class AnimationControllerSerializer
	{
	public:
		static bool Serialize(const std::filesystem::path& filepath, const SharedPtr<AnimationController>& animationController);
		static SharedPtr<AnimationController> Deserialize(UUID uuid, const std::filesystem::path& filepath);
	};

}
