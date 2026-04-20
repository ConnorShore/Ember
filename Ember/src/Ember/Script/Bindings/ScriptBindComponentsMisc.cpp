#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"

namespace Ember {
	void BindMiscComponents(sol::state& state)
	{
		state.new_usertype<AnimatorComponent>("AnimatorComponent",
			"CurrentAnimationHandle", &AnimatorComponent::CurrentAnimationHandle,
			"CurrentTime", sol::property(
				[](AnimatorComponent& c) { return (float)c.CurrentTime; },
				[](AnimatorComponent& c, float time) { c.CurrentTime = time; }
			),
			"PlaybackSpeed", &AnimatorComponent::PlaybackSpeed,
			"IsPlaying", &AnimatorComponent::IsPlaying,
			"Loop", &AnimatorComponent::Loop,
			"Crossfade", [](AnimatorComponent& c, UUID targetAnim, float duration) {
				if (c.CurrentAnimationHandle == targetAnim || targetAnim == Constants::InvalidUUID)
					return;
				if (c.CurrentAnimationHandle == Constants::InvalidUUID)
				{
					c.CurrentAnimationHandle = targetAnim;
					c.CurrentTime = 0.0f;
					c.IsPlaying = true;
					return;
				}

				c.PreviousAnimationHandle = c.CurrentAnimationHandle;
				c.PreviousTime = c.CurrentTime;
				c.CurrentAnimationHandle = targetAnim;
				c.CurrentTime = 0.0f;
				c.BlendDuration = duration;
				c.CurrentBlendTime = 0.0f;
				c.IsPlaying = true;
			}
		);

		state.new_usertype<LifetimeComponent>("LifetimeComponent",
			"Lifetime", &LifetimeComponent::Lifetime
		);
	}
}