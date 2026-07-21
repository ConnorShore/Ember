#include "ebpch.h"
#include "ScriptBindComponents.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/ECS/System/AISystem.h"
#include "Ember/Script/Bindings/ScriptComponentRef.h"

namespace Ember {

	void BindAIComponents(sol::state& state, Scene* scene)
	{
		// Bound as a resolving handle (see ScriptComponentRef.h): safe to cache in Lua.
		state.new_usertype<ComponentRef<AIPathComponent>>("AIPathComponent",
			"Waypoints", RefProp(&AIPathComponent::Waypoints),
			"CurrentWaypointIndex", RefProp(&AIPathComponent::CurrentWaypointIndex),
			"Speed", RefProp(&AIPathComponent::Speed),
			"ArrivalTolerance", RefProp(&AIPathComponent::ArrivalTolerance),
			"GetNextWaypointPosition", [scene](ComponentRef<AIPathComponent>& r) -> Vector3f
			{
				AIPathComponent& c = r.Resolve();
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

		state.new_usertype<ComponentRef<AIAgentComponent>>("AIAgentComponent",
			"Mode", sol::property(
				[](ComponentRef<AIAgentComponent>& r) {
					return r.Resolve().Mode;
				},
				[](ComponentRef<AIAgentComponent>& r, AIAgentComponent::PathMode mode) {
					auto& c = r.Resolve();
					c.Mode = mode;
					c.Dirty = true;
				}
			),
			"Waypoints", RefProp(&AIAgentComponent::ManualWaypoints),
			"Loop", RefProp(&AIAgentComponent::Loop),
			"TargetEntity", RefProp(&AIAgentComponent::TargetEntity),
			"GridEntity", RefProp(&AIAgentComponent::GridEntity),
			"RecalculateInterval", RefProp(&AIAgentComponent::RecalculateInterval)
		);

		state.new_usertype<ComponentRef<LocalAvoidanceComponent>>("LocalAvoidanceComponent",
			"AvoidanceRadius", RefProp(&LocalAvoidanceComponent::AvoidanceRadius),
			"AvoidanceStrength", RefProp(&LocalAvoidanceComponent::AvoidanceStrength),
			"AvoidanceVector", RefProp(&LocalAvoidanceComponent::AvoidanceVector),
			"AvoidanceMask", RefProp(&LocalAvoidanceComponent::AvoidanceMask)
		);
	}

}