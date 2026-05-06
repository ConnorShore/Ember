#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/ECS/System/AISystem.h"

namespace Ember {

	void BindAIComponents(sol::state& state, Scene* scene)
	{
		state.new_usertype<AIPathComponent>("AIPathComponent",
			"Waypoints", &AIPathComponent::Waypoints,
			"CurrentWaypointIndex", &AIPathComponent::CurrentWaypointIndex,
			"Speed", &AIPathComponent::Speed,
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

		state.new_enum("PathMode",
			"Manual", AIAgentComponent::PathMode::Manual,
			"Dynamic", AIAgentComponent::PathMode::Dynamic
		);

		state.new_usertype<AIAgentComponent>("AIAgentComponent",
			"Mode", sol::property(
				[](AIAgentComponent& c) {
					return c.Mode;
				},
				[scene](AIAgentComponent& c, AIAgentComponent::PathMode mode) {
					c.Mode = mode;
					c.Dirty = true;
				}
			),
			"Waypoints", &AIAgentComponent::ManualWaypoints,
			"Loop", &AIAgentComponent::Loop,
			"TargetEntity", &AIAgentComponent::TargetEntity,
			"GridEntity", &AIAgentComponent::GridEntity,
			"RecalculateInterval", &AIAgentComponent::RecalculateInterval
		);

		state.new_usertype<LocalAvoidanceComponent>("LocalAvoidanceComponent",
			"AvoidanceRadius", &LocalAvoidanceComponent::AvoidanceRadius,
			"AvoidanceStrength", &LocalAvoidanceComponent::AvoidanceStrength,
			"AvoidanceVector", &LocalAvoidanceComponent::AvoidanceVector,
			"AvoidanceMask", &LocalAvoidanceComponent::AvoidanceMask
		);
	}

}