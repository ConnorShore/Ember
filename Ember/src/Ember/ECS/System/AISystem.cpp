#include "ebpch.h"
#include "AISystem.h"

#include "Ember/Render/DebugRenderer.h"

namespace Ember {

	AISystem::AISystem()
	{

	}

	AISystem::~AISystem()
	{

	}

	void AISystem::OnAttach()
	{

	}

	void AISystem::OnUpdate(TimeStep delta, Scene* scene)
	{

	}

	void AISystem::OnDetach()
	{

	}

	void AISystem::OnEditorUpdate(TimeStep delta, Scene* scene)
	{
		auto& registry = scene->GetRegistry();

		// Define some colors
		Vector4f unselectedColor(0.2f, 0.2f, 0.2f, 0.5f); // Faint dark grey
		Vector4f selectedColor(1.0f, 1.0f, 0.0f, 1.0f);   // Bright yellow
		Vector4f waypointColor(0.0f, 1.0f, 1.0f, 1.0f);   // Cyan for individual waypoints

		// We will track which paths should be highlighted
		std::vector<EntityID> pathsToHighlight;

		// 1. Determine what is currently selected
		if (m_PreviewPathEntity != Constants::Entities::InvalidEntityID)
		{
			Entity selectedEntity(m_PreviewPathEntity, scene);

			// If the AI itself is selected, flag its path to be highlighted
			if (selectedEntity.ContainsComponent<AIPathComponent>())
			{
				pathsToHighlight.push_back(selectedEntity.GetEntityHandle());
			}

			// If a Waypoint is selected, find EVERY path that uses this waypoint and highlight them!
			if (selectedEntity.ContainsComponent<WaypointComponent>())
			{
				// Draw a sphere around the selected waypoint
				auto& wpTransform = selectedEntity.GetComponent<TransformComponent>();
				// Assuming you have a DrawSphere or DrawBox in your DebugRenderer
				DebugRenderer::DrawOctahedron(wpTransform.GetWorldTransform()[3], 0.5f, waypointColor);

				auto view = registry.ActiveQuery<AIPathComponent>();
				for (EntityID e : view)
				{
					auto& pathComp = registry.GetComponent<AIPathComponent>(e);
					// If this path contains the selected waypoint, highlight the whole path
					if (std::find(pathComp.Waypoints.begin(), pathComp.Waypoints.end(), selectedEntity.GetUUID()) != pathComp.Waypoints.end())
					{
						pathsToHighlight.push_back(e);
					}
				}
			}
		}

		// 2. Draw all paths!
		auto view = registry.ActiveQuery<AIPathComponent>();
		for (EntityID e : view)
		{
			auto& pathComp = registry.GetComponent<AIPathComponent>(e);

			// Skip empty paths
			if (pathComp.Waypoints.size() < 2)
				continue;

			// Check if this path was flagged for highlighting
			bool isHighlighted = std::find(pathsToHighlight.begin(), pathsToHighlight.end(), e) != pathsToHighlight.end();
			Vector4f lineColor = isHighlighted ? selectedColor : unselectedColor;

			// Draw the lines connecting the waypoints
			for (size_t i = 0; i < pathComp.Waypoints.size(); i++)
			{
				UUID currentWP = pathComp.Waypoints[i];
				UUID nextWP = Constants::InvalidUUID;

				// Handle looping vs non-looping paths
				if (i < pathComp.Waypoints.size() - 1)
				{
					nextWP = pathComp.Waypoints[i + 1];
				}
				else if (pathComp.Loop) // If it loops, connect the last point back to the first
				{
					nextWP = pathComp.Waypoints[0];
				}

				// If we have a valid segment, draw it
				if (nextWP != Constants::InvalidUUID)
				{
					Entity currentWPEntity = scene->GetEntity(currentWP);
					Entity nextWPEntity = scene->GetEntity(nextWP);

					if (currentWPEntity.ContainsComponent<TransformComponent>() &&
						nextWPEntity.ContainsComponent<TransformComponent>())
					{
						Vector3f startPos = currentWPEntity.GetComponent<TransformComponent>().WorldTransform[3];
						Vector3f endPos = nextWPEntity.GetComponent<TransformComponent>().WorldTransform[3];

						DebugRenderer::DrawLine(startPos, endPos, lineColor);

						// If highlighted, draw spheres at the connection points
						if (isHighlighted)
						{
							 DebugRenderer::DrawOctahedron(startPos, 0.3f, selectedColor);
						}
					}
				}
			}
		}
	}

}