#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"

namespace Ember {
	void BindMiscComponents(sol::state& state)
	{
		state.new_usertype<AnimatorComponent>("AnimatorComponent",
			"SkeletonHandle", &AnimatorComponent::SkeletonHandle,
			"ControllerHandle", &AnimatorComponent::ControllerHandle,
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

		state.new_usertype<PrefabComponent>("PrefabComponent",
			"PrefabHandle", &PrefabComponent::PrefabHandle
		);
	}
}