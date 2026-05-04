#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"

namespace Ember {

	void BindAIComponents(sol::state& state, Scene* scene)
	{
		state.new_usertype<AIPathComponent>("AIPathComponent",
			"Waypoints", &AIPathComponent::Waypoints,
			"CurrentWaypointIndex", &AIPathComponent::CurrentWaypointIndex,
			"Speed", &AIPathComponent::Speed,
			"Loop", &AIPathComponent::Loop,
			"ArrivalTolerance", &AIPathComponent::ArrivalTolerance,
			"GetNextWaypointPosition", [scene](AIPathComponent& c) -> Vector3f
			{
				if (scene == nullptr)
					return {};
				if (c.CurrentWaypointIndex >= c.Waypoints.size())
				{
					EB_CORE_WARN("CurrentWaypointIndex {} is out of bounds for Waypoints of size {}", c.CurrentWaypointIndex, c.Waypoints.size());
					return {};
				}

				UUID wpID = c.Waypoints[c.CurrentWaypointIndex];
				Entity wpEntity = scene->GetEntity(wpID);
				if (wpEntity == Constants::Entities::InvalidEntityID)
					return {};
				if (!wpEntity.ContainsComponent<TransformComponent>())
					return {};
				auto& transform = wpEntity.GetComponent<TransformComponent>();
				return transform.GetWorldTransform()[3]; // Return the translation part of the world transform as the waypoint position
			}
		);
	}

}