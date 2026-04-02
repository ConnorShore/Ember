#pragma once

#include "ComponentUI.h"
#include "UI/PropertyGrid.h"

namespace Ember {

	class PointLightComponentUI : public ComponentUI<PointLightComponent>
	{
	public:
		PointLightComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Point Light Component"; }

	protected:
		inline void RenderComponentImpl(PointLightComponent& component) override
		{
			if (UI::PropertyGrid::Begin("PointLightProps"))
			{
				UI::PropertyGrid::Color3("Color", component.Color);
				UI::PropertyGrid::Float("Intensity", component.Intensity, 1.0f, 0.0f, m_MaxValue);

				float radiusValue = m_UseRadius ? 1.0f : 0.0f;

				if (UI::PropertyGrid::Checkbox("Use Radius", m_UseRadius))
				{
					if (m_UseRadius)
						component.Radius = 10.0f;
					else
						component.Radius = 0.0f;
				}
				if (m_UseRadius)
					UI::PropertyGrid::Float("Cutoff Radius", component.Radius, 0.5f, 0.001f, m_MaxValue);
				
				UI::PropertyGrid::End();
			}
		}

	private:
		bool m_UseRadius = false;
		float m_MaxValue = 999999.9f;
	};

}