#pragma once

#include "ComponentUI.h"

namespace Ember {

	class PointLightComponentUI : public ComponentUI<PointLightComponent>
	{
	public:
		inline const std::string& GetName() const override { return "Point Light Component"; }

	protected:
		inline void DrawComponentImpl(PointLightComponent& component) override
		{
			ImGui::Text("Point Light Component Data goes here");
		}

	private:
	};

}