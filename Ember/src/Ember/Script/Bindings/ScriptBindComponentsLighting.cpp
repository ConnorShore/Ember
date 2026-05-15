#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"

namespace Ember {
	void BindLightingAndCameraComponents(sol::state& state)
	{
		state.new_usertype<DirectionalLightComponent>("DirectionalLightComponent",
			"IsActive", &DirectionalLightComponent::Active,
			"Color", &DirectionalLightComponent::Color,
			"Intensity", &DirectionalLightComponent::Intensity
		);

		state.new_usertype<PointLightComponent>("PointLightComponent",
			"IsActive", &PointLightComponent::Active,
			"Color", &PointLightComponent::Color,
			"Intensity", &PointLightComponent::Intensity,
			"Radius", &PointLightComponent::Radius
		);

		state.new_usertype<SpotLightComponent>("SpotLightComponent",
			"IsActive", &SpotLightComponent::Active,
			"Color", &SpotLightComponent::Color,
			"Intensity", &SpotLightComponent::Intensity,
			"CutOffAngle", &SpotLightComponent::CutOffAngle,
			"OuterCutOffAngle", &SpotLightComponent::CutOffAngle
		);
	}
}