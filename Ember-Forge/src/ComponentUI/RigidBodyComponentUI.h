#pragma once

#include "ComponentUI.h"

namespace Ember {

	class RigidBodyComponentUI : public ComponentUI<RigidBodyComponent>
	{
	public:
		inline const std::string& GetName() const override { return "RigidBody Component"; }

	protected:
		inline void DrawComponentImpl(RigidBodyComponent& component) override
		{
			ImGui::Text("RigidBody data goes here...");
		}

	private:
	};

}