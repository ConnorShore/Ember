#pragma once

#include "ComponentUI.h"
#include "UI/UIWidgets.h"
#include "UI/PropertyGrid.h"

#include <imgui/imgui.h>

namespace Ember {

	class CharacterControllerComponentUI : public ComponentUI<CharacterControllerComponent>
	{
	public:
		CharacterControllerComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Character Controller Component"; }

	protected:
		inline void RenderComponentImpl(CharacterControllerComponent& component) override
		{
			if (UI::PropertyGrid::Begin("CharacterControllerProps"))
			{
				ImGui::TextDisabled("Movement Properties");
				UI::PropertyGrid::Float("Walk Speed", component.WalkSpeed);
				UI::PropertyGrid::Float("Jump Force", component.JumpForce);

				ImGui::Separator();

				ImGui::TextDisabled("Physics Properties");
				UI::PropertyGrid::Float("Gravity Multiplier", component.GravityMultiplier);
				UI::PropertyGrid::Float("Max Slope Angle", component.MaxSlopeAngle);
				UI::PropertyGrid::Float("Max Step Height", component.MaxStepHeight);

				UI::PropertyGrid::End();
			}
		}
	};

}