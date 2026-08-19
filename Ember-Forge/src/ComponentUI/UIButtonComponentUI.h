#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"

#include <imgui/imgui.h>

namespace Ember {

	class UIButtonComponentUI : public ComponentUI<UIButtonComponent>
	{
	public:
		UIButtonComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "UI Button Component"; }

	protected:
		inline void RenderComponentImpl(UIButtonComponent& component) override
		{
			if (!UI::PropertyGrid::Begin("UIButtonProps"))
				return;

			// The button itself carries no authored state; interaction lives on UISelectableComponent
			// and click handlers are registered from Lua.
			UI::PropertyGrid::LabelWithValue("Click Handler", "OnClick(entity) in a script, or button:OnClick(fn)");

			if (!m_Context->ActiveScene()->GetRegistry().ContainsComponent<UISelectableComponent>(m_Context->SelectedEntity))
				UI::PropertyGrid::LabelWithValue("Warning", "Requires a UI Selectable Component to respond to input");

			UI::PropertyGrid::End();
		}
	};

}
