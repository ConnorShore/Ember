#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"

namespace Ember {
	void BindMiscComponents(sol::state& state)
	{
		state.new_usertype<AnimatorComponent>("AnimatorComponent",
			"SkeletonHandle", &AnimatorComponent::SkeletonHandle,
			"AnimationStateMachineHandle", &AnimatorComponent::AnimationStateMachineHandle,
			"CurrentStateName", &AnimatorComponent::CurrentStateName,
			"PreviousStateName", &AnimatorComponent::PreviousStateName,
			"CurrentTime", sol::property(
				[](AnimatorComponent& c) { return c.CurrentTime.Seconds(); },
				[](AnimatorComponent& c, float time) { c.CurrentTime = time; }
			),
			"PreviousTime", sol::property(
				[](AnimatorComponent& c) { return c.PreviousTime.Seconds(); },
				[](AnimatorComponent& c, float time) { c.PreviousTime = time; }
			),
			"CurrentBlendTime", &AnimatorComponent::CurrentBlendTime,
			"ActiveBlendDuration", &AnimatorComponent::ActiveBlendDuration,
			"IsBlending", &AnimatorComponent::IsBlending,
			"SetFloat", &AnimatorComponent::SetFloat,
			"SetBool", &AnimatorComponent::SetBool
		);

		state.new_usertype<BoneSocketComponent>("BoneSocketComponent",
			"TargetEntityHandle", &BoneSocketComponent::TargetEntityHandle,
			"BoneName", &BoneSocketComponent::BoneName,
			"Position", &BoneSocketComponent::Position,
			"Rotation", &BoneSocketComponent::Rotation,
			"Scale", &BoneSocketComponent::Scale
		);

		state.new_usertype<LifetimeComponent>("LifetimeComponent",
			"Lifetime", &LifetimeComponent::Lifetime
		);
	}
}