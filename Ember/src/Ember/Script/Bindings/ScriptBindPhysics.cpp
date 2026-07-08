#include "ebpch.h"
#include "ScriptBindPhysics.h"

#include "Ember/Core/ProjectManager.h"
#include "Ember/Math/Math.h"
#include "Ember/Physics/Raycast.h"
#include "Ember/Physics/Collision.h"

namespace Ember {

	struct LuaRaycastHit
	{
		bool Hit = false;
		Vector3f CollisionPoint = { 0.0f, 0.0f, 0.0f };
		Vector3f SurfaceNormal = { 0.0f, 0.0f, 0.0f }; 
		Entity RigidBodyEntity;
		Entity ColliderEntity;
	};

	void BindPhysics(sol::state& state, Scene* scene)
	{
		state.new_usertype<FilterManager>("CollisionFilterManager",
			"SetFilterNameAtSlot", [](uint32_t slot, const std::string& name) {
				return ProjectManager::GetActive()->GetCollisionFilterManager().SetFilterNameAtSlot(slot, name);
			},
			"GetFilter", [](const std::string& name) {
				return ProjectManager::GetActive()->GetCollisionFilterManager().GetFilter(name);
			},
			"GetFilterNameBySlot", [](uint32_t slot) {
				return ProjectManager::GetActive()->GetCollisionFilterManager().GetFilterNameBySlot(slot);
			}
		);

		sol::table collisionFilterTable = state.create_table("CollisionFilter");

		// Defaults
		collisionFilterTable["Default"] = FilterPreset::Default;
		collisionFilterTable["All"] = FilterPreset::All;

		// Custom filter bindings
		auto& filterManager = ProjectManager::GetActive()->GetCollisionFilterManager();
		for (int i = 0; i < 16; i++) {
			std::string customName = filterManager.GetFilterNameBySlot(i);
			if (!customName.empty()) {
				collisionFilterTable[customName] = (1 << i);
			}
		}

		state.new_usertype<LuaRaycastHit>("RaycastHit",
			"Hit", &LuaRaycastHit::Hit,
			"CollisionPoint", &LuaRaycastHit::CollisionPoint,
			"SurfaceNormal", &LuaRaycastHit::SurfaceNormal, 
			"RigidBodyEntity", &LuaRaycastHit::RigidBodyEntity,
			"ColliderEntity", &LuaRaycastHit::ColliderEntity
		);

		state.new_usertype<Hit>("Hit",
			"Entity", &Hit::EntityID,
			"Filter", &Hit::Filter
		);

		state.new_usertype<OverlapTestData>("OverlapData",
			"Hits", &OverlapTestData::Hits
		);

		// Create the Physics static table
		auto physicsTable = state.create_table("Physics");
		physicsTable.set_function("CastRay", [scene](const Vector3f& start, const Vector3f& end, sol::optional<Filter> filterOpt) {

			Filter filter = filterOpt.value_or(FilterPreset::All);
			RaycastData rawData = Raycast::CastRay(start, end, filter);

			LuaRaycastHit luaHit;
			luaHit.Hit = rawData.Hit;
			luaHit.CollisionPoint = rawData.CollisionPoint;
			luaHit.SurfaceNormal = rawData.SurfaceNormal;

			// Wrap BOTH entities so Lua can use :ContainsComponent() on them!
			if (rawData.Hit)
			{
				if (rawData.RigidBodyEntity != Constants::Entities::InvalidEntityID)
					luaHit.RigidBodyEntity = Entity(rawData.RigidBodyEntity, scene);

				if (rawData.ColliderEntity != Constants::Entities::InvalidEntityID)
					luaHit.ColliderEntity = Entity(rawData.ColliderEntity, scene);
			}

			return luaHit;
			});
		physicsTable.set_function("CheckOverlapBox", sol::overload(
			[](const Vector3f& position, const Vector3f& rotation, const Vector3f& scale, Entity entity) {
				return Collision::CheckOverlapBox(position, rotation, scale, entity);
			},
			[](const Vector3f& position, const Vector3f& rotation, const Vector3f& scale, Entity entity, Filter filter) {
				return Collision::CheckOverlapBox(position, rotation, scale, entity, filter);
			}
		));
		physicsTable.set_function("CheckOverlapBoxWithData", sol::overload(
			[](const Vector3f& position, const Vector3f& rotation, const Vector3f& scale, Entity entity) {
				return Collision::CheckOverlapBoxWithData(position, rotation, scale, entity);
			},
			[](const Vector3f& position, const Vector3f& rotation, const Vector3f& scale, Entity entity, Filter filter) {
				return Collision::CheckOverlapBoxWithData(position, rotation, scale, entity, filter);
			}
		));
		physicsTable.set_function("CheckOverlapSphere", sol::overload(
			[](Entity entity) {
				return Collision::CheckOverlapSphere(entity);
			},
			[](const Vector3f& position, float radius, Entity entity) {
				return Collision::CheckOverlapSphere(position, radius, entity);
			},
			[](const Vector3f& position, float radius, Entity entity, Filter filter) {
				return Collision::CheckOverlapSphere(position, radius, entity, filter);
			}
		));
		physicsTable.set_function("CheckOverlapSphereWithData", sol::overload(
			[](const Vector3f& position, float radius, Entity entity) {
				return Collision::CheckOverlapSphereWithData(position, radius, entity);
			},
			[](const Vector3f& position, float radius, Entity entity, Filter filter) {
				return Collision::CheckOverlapSphereWithData(position, radius, entity, filter);
			}
		));
		physicsTable.set_function("TestCollision", [](Entity entity) {
			return Collision::TestCollision(entity);
		});
	}

}
