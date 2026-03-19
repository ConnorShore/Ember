#pragma once

#include "ComponentUI.h"

namespace Ember {

	class SpotLightComponentUI : public ComponentUI<SpotLightComponent>
	{
	public:
		inline const char* GetName() const override { return "Spot Light Component"; }

	protected:
		inline void RenderComponentImpl(SpotLightComponent& component) override
		{
			ImGui::Text("Spot Light Component Data goes here");
		}

	private:
	};

}