#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Animation/AnimationStateMachine.h"

namespace Ember {

	class AnimationStateMachineSerializer
	{
	public:
		static bool Serialize(const std::filesystem::path& filepath, const SharedPtr<AnimationStateMachine>& animationStateMachine);
		static SharedPtr<AnimationStateMachine> Deserialize(UUID uuid, const std::filesystem::path& filepath);
	};

}
