#pragma once

#include "ComponentUI.h"
#include "UI/PropertyGrid.h"

namespace Ember {

	class DirectionalLightComponentUI : public ComponentUI<DirectionalLightComponent>
	{
	public:
		DirectionalLightComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Directional Light Component"; }

	protected:
		inline void RenderComponentImpl(DirectionalLightComponent& component) override
		{
			if (UI::PropertyGrid::Begin("DirectionalLightProps"))
			{
				UI::PropertyGrid::Color3("Color", component.Color);
				UI::PropertyGrid::Float("Intensity", component.Intensity, 0.1f, 0.0f, m_MaxValue);
				UI::PropertyGrid::End();
			}
		}

	private:
		float m_MaxValue = 1000.0f;
	};

}