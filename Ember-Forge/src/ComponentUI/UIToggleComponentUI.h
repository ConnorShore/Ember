#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"
#include "UI/DragDropTypes.h"

#include <imgui/imgui.h>

namespace Ember {

	class UIToggleComponentUI : public ComponentUI<UIToggleComponent>
	{
	public:
		UIToggleComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "UI Toggle Component"; }

	protected:
		inline void RenderComponentImpl(UIToggleComponent& component) override
		{
			if (!UI::PropertyGrid::Begin("UIToggleProps"))
				return;

			if (UI::PropertyGrid::Checkbox("Is On", component.IsOn))
				component.VisualStateApplied = false;

			UI::PropertyGrid::Checkbox("Allow Switch Off", component.AllowSwitchOff);

			DrawEntityReference("Checkmark", component.CheckmarkEntity);

			// Any entity works as a grouping key; toggles sharing it become mutually exclusive.
			DrawEntityReference("Group", component.GroupEntity);

			UI::PropertyGrid::End();
		}

	private:
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
