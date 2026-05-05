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

				return c.Waypoints[c.CurrentWaypointIndex];
			}
		);
	}

}