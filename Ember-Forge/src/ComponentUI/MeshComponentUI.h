#pragma once

#include "ComponentUI.h"

namespace Ember {

	class MeshComponentUI : public ComponentUI<MeshComponent>
	{
	public:
		MeshComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Mesh Component"; }

	protected:
		inline void RenderComponentImpl(MeshComponent& component) override
		{
			ImGui::Text("Mesh data goes here...");
		}

	private:
	};

}