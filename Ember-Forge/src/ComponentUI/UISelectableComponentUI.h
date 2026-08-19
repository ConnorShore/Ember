#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"
#include "UI/DragDropTypes.h"

#include <imgui/imgui.h>

namespace Ember {

	class UISelectableComponentUI : public ComponentUI<UISelectableComponent>
	{
	public:
		UISelectableComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "UI Selectable Component"; }

	protected:
		inline void RenderComponentImpl(UISelectableComponent& component) override
		{
			if (!UI::PropertyGrid::Begin("UISelectableProps"))
				return;

			UI::PropertyGrid::Checkbox("Interactable", component.Interactable);

			static const char* transitionNames[] = { "None", "Color Tint", "Sprite Swap" };
			if (UI::PropertyGrid::BeginComboBox("Transition", transitionNames[(int)component.Transition]))
			{
				for (int i = 0; i < IM_ARRAYSIZE(transitionNames); i++)
				{
					if (UI::PropertyGrid::ComboBoxItem(transitionNames[i], (int)component.Transition == i))
						component.Transition = (UITransitionMode)i;
				}
				UI::PropertyGrid::EndComboBox();
			}

			DrawEntityReference("Target Graphic", component.TargetGraphicEntity);

			if (component.Transition == UITransitionMode::ColorTint)
			{
				UI::PropertyGrid::Color4("Normal Color", component.NormalColor);
				UI::PropertyGrid::Color4("Highlighted Color", component.HighlightedColor);
				UI::PropertyGrid::Color4("Pressed Color", component.PressedColor);
				UI::PropertyGrid::Color4("Selected Color", component.SelectedColor);
				UI::PropertyGrid::Color4("Disabled Color", component.DisabledColor);
				UI::PropertyGrid::Float("Fade Duration", component.FadeDuration, 0.01f, 0.0f, 5.0f);
			}

			static const char* navigationNames[] = { "None", "Automatic", "Explicit" };
			if (UI::PropertyGrid::BeginComboBox("Navigation", navigationNames[(int)component.Navigation]))
			{
				for (int i = 0; i < IM_ARRAYSIZE(navigationNames); i++)
				{
					if (UI::PropertyGrid::ComboBoxItem(navigationNames[i], (int)component.Navigation == i))
						component.Navigation = (UINavigationMode)i;
				}
				UI::PropertyGrid::EndComboBox();
			}

			if (component.Navigation == UINavigationMode::Explicit)
			{
				DrawEntityReference("Navigate Up", component.NavigateUp);
				DrawEntityReference("Navigate Down", component.NavigateDown);
				DrawEntityReference("Navigate Left", component.NavigateLeft);
				DrawEntityReference("Navigate Right", component.NavigateRight);
			}

			UI::PropertyGrid::End();
		}

	private:
		// Resolves the current reference to a name and accepts a hierarchy drag-drop onto it.
		inline void DrawEntityReference(const char* label, UUID& target)
		{
			std::string targetName = "None";
			if (target != Constants::InvalidUUID)
			{
				Entity targetEntity = m_Context->ActiveScene()->GetEntity(target);
				targetName = targetEntity == Constants::Entities::InvalidEntityID ? "Invalid Entity" : targetEntity.GetName();
			}

			std::string payloadStr = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::SceneEntity);
			UUID droppedUUID = Constants::InvalidUUID;
			if (UI::PropertyGrid::EntityReference(label, targetName, payloadStr, droppedUUID, [&]() { target = Constants::InvalidUUID; }))
			{
				Entity dropped = m_Context->ActiveScene()->GetEntity(droppedUUID);
				if (dropped != Constants::Entities::InvalidEntityID && dropped.GetUUID() != Constants::InvalidUUID)
					target = dropped.GetUUID();
			}
		}
	};

}
