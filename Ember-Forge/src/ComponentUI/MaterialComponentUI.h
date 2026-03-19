#pragma once

#include "ComponentUI.h"

namespace Ember {

	class MaterialComponentUI : public ComponentUI<MaterialComponent>
	{
	public:
		inline const char* GetName() const override { return "Material Component"; }

	protected:
		inline void RenderComponentImpl(MaterialComponent& component) override
		{
			ImGui::Text("Material data goes here...");
		}

	private:
	};

}