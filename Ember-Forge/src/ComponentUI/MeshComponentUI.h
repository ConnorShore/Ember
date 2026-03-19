#pragma once

#include "ComponentUI.h"

namespace Ember {

	class MeshComponentUI : public ComponentUI<MeshComponent>
	{
	public:
		inline const std::string& GetName() const override { return "Mesh Component"; }

	protected:
		inline void DrawComponentImpl(MeshComponent& component) override
		{
			ImGui::Text("Mesh data goes here...");
		}

	private:
	};

}