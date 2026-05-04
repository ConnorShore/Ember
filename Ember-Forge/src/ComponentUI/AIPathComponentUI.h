#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"

namespace Ember {

	class AIPathComponentUI : public ComponentUI<AIPathComponent>
	{
	public:
		AIPathComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "A.I. Path Component"; }

	protected:
		inline void RenderComponentImpl(AIPathComponent& component) override
		{
			if (UI::PropertyGrid::Begin("AIPathProps"))
			{
				UI::PropertyGrid::Float("Speed", component.Speed);
				UI::PropertyGrid::Float("Arrival Tolerance", component.ArrivalTolerance, 0.01f, 0.0f, 100.0f);

				// --- Waypoint Array Header ---
				std::string caption = std::to_string(component.Waypoints.size()) + " points";
				if (UI::PropertyGrid::HeaderWithActionButton("Waypoints", "+", caption))
				{
					// Add an empty entity slot to the path
					component.Waypoints.push_back(Constants::InvalidUUID);
				}

				int elementToRemove = -1;

				// --- Waypoint Array Elements ---
				for (size_t i = 0; i < component.Waypoints.size(); i++)
				{
					UUID wpID = component.Waypoints[i];
					std::string label = "  Point " + std::to_string(i);

					std::string entityName = "None";
					if (wpID != Constants::InvalidUUID)
					{
						Entity entity = m_Context->ActiveScene->GetEntity(wpID);
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
						Entity droppedEnt = m_Context->ActiveScene->GetEntity(droppedUUID);
						if (droppedEnt.GetUUID() != Constants::InvalidUUID)
						{
							component.Waypoints[i] = droppedEnt.GetUUID();
						}
					}
				}

				// Safely remove the element if the clear button was clicked
				if (elementToRemove >= 0)
				{
					component.Waypoints.erase(component.Waypoints.begin() + elementToRemove);
					if (component.CurrentWaypointIndex >= component.Waypoints.size())
						component.CurrentWaypointIndex = 0;
				}

				UI::PropertyGrid::End();
			}
		}
	};

}