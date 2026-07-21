#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Script/Bindings/ScriptComponentRef.h"

namespace Ember {
	void BindLightingAndCameraComponents(sol::state& state)
	{
		// Bound as resolving handles (see ScriptComponentRef.h): safe to cache in Lua.
		state.new_usertype<ComponentRef<DirectionalLightComponent>>("DirectionalLightComponent",
			"IsActive", RefProp(&DirectionalLightComponent::Active),
			"Color", RefProp(&DirectionalLightComponent::Color),
			"Intensity", RefProp(&DirectionalLightComponent::Intensity)
		);

		state.new_usertype<ComponentRef<PointLightComponent>>("PointLightComponent",
			"IsActive", RefProp(&PointLightComponent::Active),
			"Color", RefProp(&PointLightComponent::Color),
			"Intensity", RefProp(&PointLightComponent::Intensity),
			"Radius", RefProp(&PointLightComponent::Radius)
		);

		state.new_usertype<ComponentRef<SpotLightComponent>>("SpotLightComponent",
			"IsActive", RefProp(&SpotLightComponent::Active),
			"Color", RefProp(&SpotLightComponent::Color),
			"Intensity", RefProp(&SpotLightComponent::Intensity),
			"CutOffAngle", RefProp(&SpotLightComponent::CutOffAngle),
			"OuterCutOffAngle", RefProp(&SpotLightComponent::CutOffAngle)
		);
	}
}