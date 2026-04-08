#pragma once


#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"

#include <imgui/imgui.h>

namespace Ember {

	class AnimatorComponentUI : public ComponentUI<AnimatorComponent>
	{
	public:
		AnimatorComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Animator Component"; }

	protected:
		inline void RenderComponentImpl(AnimatorComponent& component) override
		{
			ImGui::Text("Animator component data here...");
		}
	};

}