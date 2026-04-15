#pragma once

#include "RaycastData.h"

#include <reactphysics3d/collision/RaycastInfo.h>

namespace Ember {

	// Exact same class as reactphysics3d::RaycastINfo,
	// just used to copy values across since the copy constructor is deleted
	struct RaycastInfoWrapper
	{
		reactphysics3d::Vector3 worldPoint;
		reactphysics3d::Vector3 worldNormal;
		reactphysics3d::decimal hitFraction;
		reactphysics3d::Body* body;
		reactphysics3d::Collider* collider;
		int triangleIndex;
	};

	class RaycastCallback : public reactphysics3d::RaycastCallback
	{
	public:
		inline virtual reactphysics3d::decimal notifyRaycastHit(const reactphysics3d::RaycastInfo& raycastInfo) override
		{
			m_HasHit = true;
			m_Info.worldPoint = raycastInfo.worldPoint;
			m_Info.worldNormal = raycastInfo.worldNormal;
			m_Info.hitFraction = raycastInfo.hitFraction;
			m_Info.body = raycastInfo.body;
			m_Info.collider = raycastInfo.collider;

			// Returning hitFraction shrinks the ray to ensure we get the CLOSEST hit!
			return raycastInfo.hitFraction;
		}

		const RaycastInfoWrapper& GetInfo() const { return m_Info; }
		const bool HasHit() const { return m_HasHit; }

	private:
		RaycastInfoWrapper m_Info;
		bool m_HasHit = false;
	};

}