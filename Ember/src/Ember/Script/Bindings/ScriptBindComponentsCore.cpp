#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Script/Bindings/ScriptComponentRef.h"

namespace Ember {
	void BindCoreComponents(sol::state& state)
	{
		// Bound as a resolving handle (see ScriptComponentRef.h): safe to cache in Lua.
		state.new_usertype<ComponentRef<TransformComponent>>("TransformComponent",
			"Position", RefProp(&TransformComponent::Position),
			"Rotation", RefProp(&TransformComponent::Rotation),
			"Scale", RefProp(&TransformComponent::Scale),
			"WorldPosition", sol::property([](ComponentRef<TransformComponent>& r) { auto& c = r.Resolve(); return Vector3f(c.GetWorldTransform()[3]); }),
			"WorldRotation", sol::property([](ComponentRef<TransformComponent>& r) { auto& c = r.Resolve(); return Math::ToEulerAngles(glm::quat_cast(c.GetWorldTransform())); }),
			"GetForward", RefMethod(&TransformComponent::GetForward),
			"GetRight", RefMethod(&TransformComponent::GetRight),
			"GetUp", RefMethod(&TransformComponent::GetUp)
		);

		state.new_usertype<ComponentRef<RelationshipComponent>>("RelationshipComponent",
			"Parent", RefProp(&RelationshipComponent::ParentHandle),
			"Children", RefProp(&RelationshipComponent::Children)
		);
	}
}