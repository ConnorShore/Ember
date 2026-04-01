#pragma once

#include "ComponentUI.h"
#include "UI/PropertyGrid.h"

namespace Ember {

	class SpotLightComponentUI : public ComponentUI<SpotLightComponent>
	{
	public:
		SpotLightComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Spot Light Component"; }

	protected:
		inline void RenderComponentImpl(SpotLightComponent& component) override
		{
			if (UI::PropertyGrid::Begin("SpotLightProps"))
			{
				UI::PropertyGrid::Color3("Color", component.Color);
				UI::PropertyGrid::Float("Intensity", component.Intensity, 1.0f, 0.0f, m_MaxValue);
				float innerCutDegrees = Math::Degrees(component.CutOffAngle);
				float outerCutDegrees = Math::Degrees(component.OuterCutOffAngle);
				if (UI::PropertyGrid::Float("Inner Cut Off Angle", innerCutDegrees, 0.5f, 0.0f, outerCutDegrees - 0.1f))
				{
					component.CutOffAngle = Math::Radians(innerCutDegrees);
					component.CutOff = cos(component.CutOffAngle);
				}
				if (UI::PropertyGrid::Float("Outer Cut Off Angle", outerCutDegrees, 0.5f, innerCutDegrees + 0.1f, 89.9f))
				{
					component.OuterCutOffAngle = Math::Radians(outerCutDegrees);
					component.OuterCutOff = cos(component.OuterCutOffAngle);
				}
				UI::PropertyGrid::End();
			}
		}

	private:
		float m_MaxValue = 999999.9f;
	};

}