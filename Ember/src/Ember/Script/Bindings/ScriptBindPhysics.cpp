#include "ebpch.h"
#include "ScriptBindPhysics.h"

#include "Ember/Core/ProjectManager.h"
#include "Ember/Math/Math.h"
#include "Ember/Physics/CollisionFilterManager.h"
#include "Ember/Physics/Raycast.h"
#include "Ember/Physics/Collision.h"

namespace Ember {

	struct LuaRaycastHit
	{
		bool Hit = false;
		Vector3f CollisionPoint = { 0.0f, 0.0f, 0.0f };
		Vector3f SurfaceNormal = { 0.0f, 0.0f, 0.0f };
		Entity HitEntity;
	};

	void BindPhysics(sol::state& state, Scene* scene)
	{
		state.new_usertype<CollisionFilterManager>("CollisionFilterManager",
			"SetFilterNameAtSlot", &CollisionFilterManager::SetFilterNameAtSlot,
			"GetFilter", &CollisionFilterManager::GetFilter,
			"GetFilterNameBySlot", &CollisionFilterManager::GetFilterNameBySlot
		);

		sol::table collisionFilterTable = state.create_table("CollisionFilter");

		// Defaults
		collisionFilterTable["Default"] = CollisionFilterPreset::Default;
		collisionFilterTable["Environment"] = CollisionFilterPreset::Environment;
		collisionFilterTable["All"] = CollisionFilterPreset::All;

		// Custom filter bindings
		auto& filterManager = ProjectManager::GetActive()->GetCollisionFilterManager();
		for (int i = 1; i < 16; i++) {
			std::string customName = filterManager.GetFilterNameBySlot(i);
			if (!customName.empty()) {
				collisionFilterTable[customName] = (1 << i);
			}
		}

		state.new_usertype<LuaRaycastHit>("RaycastHit",
			"Hit", &LuaRaycastHit::Hit,
			"CollisionPoint", &LuaRaycastHit::CollisionPoint,
			"SurfaceNormal", &LuaRaycastHit::SurfaceNormal,
			"HitEntity", &LuaRaycastHit::HitEntity
		);

		// Create the Physics static table
		auto physicsTable = state.create_table("Physics");
		physicsTable.set_function("CastRay", [scene](const Vector3f& start, const Vector3f& end) {

			RaycastData rawData = Raycast::CastRay(start, end);

			LuaRaycastHit luaHit;
			luaHit.Hit = rawData.Hit;
			luaHit.CollisionPoint = rawData.CollisionPoint;
			luaHit.SurfaceNormal = rawData.SurfaceNormal;

			if (rawData.Hit && rawData.RigidBodyEntity != Constants::Entities::InvalidEntityID)
				luaHit.HitEntity = Entity(rawData.RigidBodyEntity, scene);

			return luaHit;
			});
		physicsTable.set_function("CheckOverlapBox", sol::overload(
			[](const Vector3f& position, const Vector3f& rotation, const Vector3f& scale, Entity entity) {
				return Collision::CheckOverlapBox(position, rotation, scale, entity);
			},
			[](const Vector3f& position, const Vector3f& rotation, const Vector3f& scale, Entity entity, CollisionFilter filter) {
				return Collision::CheckOverlapBox(position, rotation, scale, entity, filter);
			}
		));
		physicsTable.set_function("CheckOverlapBoxWithData", sol::overload(
			[](const Vector3f& position, const Vector3f& rotation, const Vector3f& scale, Entity entity) {
				return Collision::CheckOverlapBoxWithData(position, rotation, scale, entity);
			},
			[](const Vector3f& position, const Vector3f& rotation, const Vector3f& scale, Entity entity, CollisionFilter filter) {
				return Collision::CheckOverlapBoxWithData(position, rotation, scale, entity, filter);
			}
		));
		physicsTable.set_function("CheckOverlapSphere", sol::overload(
			[](const Vector3f& position, float radius, Entity entity) {
				return Collision::CheckOverlapSphere(position, radius, entity);
			},
			[](const Vector3f& position, float radius, Entity entity, CollisionFilter filter) {
				return Collision::CheckOverlapSphere(position, radius, entity, filter);
			}
		));
		physicsTable.set_function("CheckOverlapSphereWithData", sol::overload(
			[](const Vector3f& position, float radius, Entity entity) {
				return Collision::CheckOverlapSphereWithData(position, radius, entity);
			},
			[](const Vector3f& position, float radius, Entity entity, CollisionFilter filter) {
				return Collision::CheckOverlapSphereWithData(position, radius, entity, filter);
			}
		));
		physicsTable.set_function("TestCollision", [](Entity entity) {
			return Collision::TestCollision(entity);
		});
	}

}
