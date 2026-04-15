#include "ebpch.h"
#include "ScriptBindPhysics.h"

#include "Ember/Math/Math.h"
#include "Ember/Physics/CollisionFilterManager.h"
#include "Ember/Physics/Raycast.h"

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

		state.new_usertype<LuaRaycastHit>("RaycastHit",
			"Hit", &LuaRaycastHit::Hit,
			"CollisionPoint", &LuaRaycastHit::CollisionPoint,
			"SurfaceNormal", &LuaRaycastHit::SurfaceNormal,
			"HitEntity", &LuaRaycastHit::HitEntity
		);

		// Create the Raycast static table
		auto raycastTable = state.create_table("Raycast");
		raycastTable.set_function("CastRay", [scene](const Vector3f& start, const Vector3f& end) {

			RaycastData rawData = Raycast::CastRay(start, end);

			LuaRaycastHit luaHit;
			luaHit.Hit = rawData.Hit;
			luaHit.CollisionPoint = rawData.CollisionPoint;
			luaHit.SurfaceNormal = rawData.SurfaceNormal;

			if (rawData.Hit && rawData.RigidBodyEntity != Constants::Entities::InvalidEntityID)
				luaHit.HitEntity = Entity(rawData.RigidBodyEntity, scene);

			return luaHit;
		});
	}

}
