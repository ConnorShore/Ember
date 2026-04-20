#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"

namespace Ember {
	void BindCoreComponents(sol::state& state)
	{
		state.new_usertype<TransformComponent>("TransformComponent",
			"Position", &TransformComponent::Position,
			"Rotation", &TransformComponent::Rotation,
			"Scale", &TransformComponent::Scale,
			"WorldPosition", sol::property([](TransformComponent& c) { return Vector3f(c.GetWorldTransform()[3]); }),
			"WorldRotation", sol::property([](TransformComponent& c) { return Math::ToEulerAngles(glm::quat_cast(c.GetWorldTransform())); }),
			"GetForward", &TransformComponent::GetForward
		);
	}
}