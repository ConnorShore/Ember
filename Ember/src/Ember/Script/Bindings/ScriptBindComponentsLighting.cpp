#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"

namespace Ember {
	void BindLightingAndCameraComponents(sol::state& state)
	{
		state.new_usertype<DirectionalLightComponent>("DirectionalLightComponent",
			"Color", &DirectionalLightComponent::Color,
			"Intensity", &DirectionalLightComponent::Intensity
		);

		state.new_usertype<PointLightComponent>("PointLightComponent",
			"Color", &PointLightComponent::Color,
			"Intensity", &PointLightComponent::Intensity,
			"Radius", &PointLightComponent::Radius
		);

		state.new_usertype<SpotLightComponent>("SpotLightComponent",
			"Color", &SpotLightComponent::Color,
			"Intensity", &SpotLightComponent::Intensity,
			"CutOffAngle", &SpotLightComponent::CutOffAngle,
			"OuterCutOffAngle", &SpotLightComponent::CutOffAngle
		);
	}
}