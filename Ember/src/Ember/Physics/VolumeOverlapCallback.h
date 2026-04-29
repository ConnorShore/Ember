#pragma once

#include "ColliderUserData.h"
#include "Ember/Math/Math.h"
#include "Ember/ECS/Types.h"

#include <vector>
#include <algorithm>

#include <reactphysics3d/collision/OverlapCallback.h>
#include <reactphysics3d/body/RigidBody.h>
#include <reactphysics3d/collision/shapes/BoxShape.h>

namespace Ember {

	struct VolumeOverlapData
	{
		EntityID CollidedEntity = Constants::Entities::InvalidEntityID;

		// Positive = Outside, 0 = On Edge, Negative = Inside
		float SignedDistanceToEdge = 0.0f;
	};

	class VolumeOverlapCallback : public reactphysics3d::OverlapCallback
	{
	public:
		VolumeOverlapCallback(const Vector3f& cameraPos, reactphysics3d::RigidBody* cameraBody) 
			: m_CameraPosition(cameraPos), m_CameraBody(cameraBody) {}

		virtual void onOverlap(reactphysics3d::OverlapCallback::CallbackData& callbackData) override
		{
			uint32_t overlapCount = callbackData.getNbOverlappingPairs();
			for (uint32_t i = 0; i < overlapCount; i++)
			{
				auto pair = callbackData.getOverlappingPair(i);
				
				// Safely filter out our persistent camera body!
				reactphysics3d::RigidBody* collidedBody = nullptr;
				if (pair.getBody1() == m_CameraBody) collidedBody = static_cast<reactphysics3d::RigidBody*>(pair.getBody2());
				else if (pair.getBody2() == m_CameraBody) collidedBody = static_cast<reactphysics3d::RigidBody*>(pair.getBody1());

				if (collidedBody == nullptr || collidedBody->getNbColliders() == 0)
					continue;

				reactphysics3d::Collider* collider = collidedBody->getCollider(0);

				if ((collider->getCollisionCategoryBits() & CollisionFilterPreset::VFX) == 0)
					continue; // If it's not a VFX volume, ignore it

				ColliderUserData* collisionData = static_cast<ColliderUserData*>(collider->getUserData());
				if (collisionData == nullptr)
					continue;

				float signedDistance = 0.0f;

				// --- SDF EDGE DISTANCE CALCULATION ---
				if (collider->getCollisionShape()->getName() == reactphysics3d::CollisionShapeName::BOX)
				{
					reactphysics3d::BoxShape* boxShape = static_cast<reactphysics3d::BoxShape*>(collider->getCollisionShape());
					reactphysics3d::Vector3 halfExtents = boxShape->getHalfExtents();

					reactphysics3d::Vector3 rp3dCamPos(m_CameraPosition.x, m_CameraPosition.y, m_CameraPosition.z);
					reactphysics3d::Transform inverseTransform = collidedBody->getTransform().getInverse();
					reactphysics3d::Vector3 localCamPos = inverseTransform * rp3dCamPos;

					float dx = std::abs(localCamPos.x) - halfExtents.x;
					float dy = std::abs(localCamPos.y) - halfExtents.y;
					float dz = std::abs(localCamPos.z) - halfExtents.z;

					float distOutside = std::sqrt(
						std::max(dx, 0.0f) * std::max(dx, 0.0f) +
						std::max(dy, 0.0f) * std::max(dy, 0.0f) +
						std::max(dz, 0.0f) * std::max(dz, 0.0f)
					);

					float distInside = std::min(std::max({ dx, dy, dz }), 0.0f);
					signedDistance = distOutside + distInside;
				}

				VolumeOverlapData data;
				data.CollidedEntity = collisionData->EntityID;
				data.SignedDistanceToEdge = signedDistance;

				m_Overlaps.push_back(data);
			}
		}

		const std::vector<VolumeOverlapData>& GetOverlaps() const { return m_Overlaps; }

	private:
		Vector3f m_CameraPosition;
		reactphysics3d::RigidBody* m_CameraBody;
		std::vector<VolumeOverlapData> m_Overlaps;
	};
}