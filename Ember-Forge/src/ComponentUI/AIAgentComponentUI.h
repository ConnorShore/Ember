#pragma once
#pragma once

#include "ComponentUI.h"
#include "UI/PropertyGrid.h"
#include "UI/DragDropTypes.h"

namespace Ember {

	class AIAgentComponentUI : public ComponentUI<AIAgentComponent>
	{
	public:
		AIAgentComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "AI Agent Component"; }

	protected:
		inline void RenderComponentImpl(AIAgentComponent& component) override
		{
			// Dropdown to select mode
			if (UI::PropertyGrid::Begin("AIAgentMode"))
			{
				if (UI::PropertyGrid::BeginComboBox("Path Mode", (component.Mode == AIAgentComponent::PathMode::Manual) ? "Manual" : "Dynamic"))
				{
					if (UI::PropertyGrid::ComboBoxItem("Manual", component.Mode == AIAgentComponent::PathMode::Manual))
					{
						component.Mode = AIAgentComponent::PathMode::Manual;
						component.Dirty = true;
					}
					if (UI::PropertyGrid::ComboBoxItem("Dynamic", component.Mode == AIAgentComponent::PathMode::Dynamic))
					{
						component.Mode = AIAgentComponent::PathMode::Dynamic;
						component.Dirty = true;
					}
					UI::PropertyGrid::EndComboBox();
				}
				UI::PropertyGrid::End();
			}

			// Render different properties based on the selected mode
			if (component.Mode == AIAgentComponent::PathMode::Manual)
				RenderManualComponentProps(component);
			else if (component.Mode == AIAgentComponent::PathMode::Dynamic)
				RenderDynamicComponentProps(component);
		}

	private:
		void RenderManualComponentProps(AIAgentComponent& component)
		{
			if (UI::PropertyGrid::Begin("AIAgentPropsManual"))
			{
				UI::PropertyGrid::Checkbox("Loop Path", component.Loop);

				// --- Waypoint Array Header ---
				std::string caption = std::to_string(component.ManualWaypoints.size()) + " points";
				if (UI::PropertyGrid::HeaderWithActionButton("Waypoints", "+", caption))
				{
					// Add an empty entity slot to the path
					component.ManualWaypoints.push_back(Constants::InvalidUUID);
				}

				int elementToRemove = -1;

				// --- Waypoint Array Elements ---
				for (size_t i = 0; i < component.ManualWaypoints.size(); i++)
				{
					UUID wpID = component.ManualWaypoints[i];
					std::string label = "  Point " + std::to_string(i);

					std::string entityName = "None";
					if (wpID != Constants::InvalidUUID)
					{
						Entity entity = m_Context->ActiveScene()->GetEntity(wpID);
						if (entity.GetUUID() != Constants::InvalidUUID)
							entityName = entity.GetName();
						else
							entityName = "Invalid Entity";
					}

					UUID droppedUUID = Constants::InvalidUUID;

					// 1. Get the EXACT string your Hierarchy panel uses for dragged entities
					std::string payloadStr = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::SceneEntity);

					// 2. Draw the target and listen for the drop
					if (UI::PropertyGrid::EntityReference(label, entityName, payloadStr, droppedUUID, [&]() { elementToRemove = (int)i; }))
					{
						// 3. Convert the UUID back to an EntityID and assign it!
						Entity droppedEnt = m_Context->ActiveScene()->GetEntity(droppedUUID);
						if (droppedEnt.GetUUID() != Constants::InvalidUUID)
						{
							component.ManualWaypoints[i] = droppedEnt.GetUUID();
						}
					}
				}

				// Safely remove the element if the clear button was clicked
				if (elementToRemove >= 0)
				{
					component.ManualWaypoints.erase(component.ManualWaypoints.begin() + elementToRemove);
				}

				UI::PropertyGrid::End();
			}
		}

		void RenderDynamicComponentProps(AIAgentComponent& component)
		{
			if (UI::PropertyGrid::Begin("AIAgentPropsDynamic"))
			{
				UI::PropertyGrid::Checkbox("Loop Path", component.Loop);
				UI::PropertyGrid::Float("Recalculate Interval", component.RecalculateInterval, 0.01f, 0.0f, 100.0f);

				// Target Entity
				std::string targetName = "None";
				if (component.TargetEntity != Constants::InvalidUUID)
				{
					Entity targetEnt = m_Context->ActiveScene()->GetEntity(component.TargetEntity);
					if (targetEnt.GetUUID() != Constants::InvalidUUID)
						targetName = targetEnt.GetName();
					else
						targetName = "Invalid Entity";
				}
				std::string payloadStr = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::SceneEntity);
				UUID droppedTargetUUID = Constants::InvalidUUID;
				if (UI::PropertyGrid::EntityReference("Target Entity", targetName, payloadStr, droppedTargetUUID))
				{
					Entity droppedEnt = m_Context->ActiveScene()->GetEntity(droppedTargetUUID);
					if (droppedEnt.GetUUID() != Constants::InvalidUUID)
						component.TargetEntity = droppedEnt.GetUUID();
				}

				// Surface/Grid Entity
				std::string gridName = "None";
				if (component.GridEntity != Constants::InvalidUUID)
				{
					Entity gridEnt = m_Context->ActiveScene()->GetEntity(component.GridEntity);
					if (gridEnt.GetUUID() != Constants::InvalidUUID)
						gridName = gridEnt.GetName();
					else
						gridName = "Invalid Entity";
				}
				UUID droppedGridUUID = Constants::InvalidUUID;
				if (UI::PropertyGrid::EntityReference("Navigation Grid/Surface", gridName, payloadStr, droppedGridUUID))
				{
					Entity droppedEnt = m_Context->ActiveScene()->GetEntity(droppedGridUUID);
					if (droppedEnt.GetUUID() != Constants::InvalidUUID)
						component.GridEntity = droppedEnt.GetUUID();
				}

				UI::PropertyGrid::End();
			}
		}
	};

}