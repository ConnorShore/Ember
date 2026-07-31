#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Script/Bindings/ScriptComponentRef.h"

namespace Ember {
	void BindMiscComponents(sol::state& state)
	{
		// Bound as a resolving handle (see ScriptComponentRef.h): safe to cache in Lua.
		state.new_usertype<ComponentRef<AnimatorComponent>>("AnimatorComponent",
			"SkeletonHandle", RefProp(&AnimatorComponent::SkeletonHandle),
			"ControllerHandle", RefProp(&AnimatorComponent::ControllerHandle),
			"PlaybackSpeed", RefProp(&AnimatorComponent::PlaybackSpeed),
			"SetFloat", RefMethod(&AnimatorComponent::SetFloat),
			"SetInt", RefMethod(&AnimatorComponent::SetInt),
			"SetBool", RefMethod(&AnimatorComponent::SetBool)
		);

		state.new_usertype<ComponentRef<BoneSocketComponent>>("BoneSocketComponent",
			"TargetEntityHandle", RefProp(&BoneSocketComponent::TargetEntityHandle),
			"BoneName", RefProp(&BoneSocketComponent::BoneName),
			"Position", RefProp(&BoneSocketComponent::Position),
			"Rotation", RefProp(&BoneSocketComponent::Rotation),
			"Scale", RefProp(&BoneSocketComponent::Scale)
		);

		state.new_usertype<ComponentRef<LifetimeComponent>>("LifetimeComponent",
			"Lifetime", RefProp(&LifetimeComponent::Lifetime)
		);

		state.new_usertype<ComponentRef<PrefabComponent>>("PrefabComponent",
			"PrefabHandle", RefProp(&PrefabComponent::PrefabHandle)
		);
	}
}