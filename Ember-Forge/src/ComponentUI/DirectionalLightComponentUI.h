#pragma once

#include "ComponentUI.h"

namespace Ember {

	class DirectionalLightComponentUI : public ComponentUI<DirectionalLightComponent>
	{
	public:
		inline const std::string& GetName() const override { return "Directional Light Component"; }

	protected:
		inline void DrawComponentImpl(DirectionalLightComponent& component) override
		{
			ImGui::Text("Directional Light Component Data goes here");
		}

	private:
	};

}