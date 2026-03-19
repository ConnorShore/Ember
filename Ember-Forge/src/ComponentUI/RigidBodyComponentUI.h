#pragma once

#include "ComponentUI.h"

namespace Ember {

	class RigidBodyComponentUI : public ComponentUI<RigidBodyComponent>
	{
	public:
		inline const char* GetName() const override { return "RigidBody Component"; }

	protected:
		inline void RenderComponentImpl(RigidBodyComponent& component) override
		{
			ImGui::Text("RigidBody data goes here...");
		}

	private:
	};

}