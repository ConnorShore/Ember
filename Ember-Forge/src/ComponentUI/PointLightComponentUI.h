#pragma once

#include "ComponentUI.h"

namespace Ember {

	class PointLightComponentUI : public ComponentUI<PointLightComponent>
	{
	public:
		PointLightComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Point Light Component"; }

	protected:
		inline void RenderComponentImpl(PointLightComponent& component) override
		{
			ImGui::Text("Point Light Component Data goes here");
		}

	private:
	};

}