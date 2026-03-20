#pragma once

#include "ComponentUI.h"

namespace Ember {

	class DirectionalLightComponentUI : public ComponentUI<DirectionalLightComponent>
	{
	public:
		DirectionalLightComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Directional Light Component"; }

	protected:
		inline void RenderComponentImpl(DirectionalLightComponent& component) override
		{
			ImGui::Text("Directional Light Component Data goes here");
		}

	private:
	};

}