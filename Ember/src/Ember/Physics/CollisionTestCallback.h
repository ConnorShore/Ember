#pragma once

#include "Ember/Math/Math.h"

#include <reactphysics3d/collision/CollisionCallback.h>
#include <vector>

namespace Ember {

	struct ContactPointData
	{
		Vector3f Point;
		Vector3f Normal;
		float PenetrationDepth = 0.0f;
	};

	struct CollisionCallbackData
	{
		bool HasHit = false;
		std::vector<ContactPointData> Contacts;
	};

	class CollisionTestCallback : public reactphysics3d::CollisionCallback
	{
	public:
		CollisionTestCallback(reactphysics3d::RigidBody* targetBody)
			: m_TargetBody(targetBody) {
		}

		void onContact(const reactphysics3d::CollisionCallback::CallbackData& callbackData) override
		{
			for (uint32_t p = 0; p < callbackData.getNbContactPairs(); p++)
			{
				reactphysics3d::CollisionCallback::ContactPair pair = callbackData.getContactPair(p);

				for (uint32_t c = 0; c < pair.getNbContactPoints(); c++)
				{
					reactphysics3d::CollisionCallback::ContactPoint contactPoint = pair.getContactPoint(c);

					m_CollisionData.HasHit = true;

					ContactPointData data;
					data.PenetrationDepth = contactPoint.getPenetrationDepth();

					reactphysics3d::Vector3 normal = contactPoint.getWorldNormal();
					if (pair.getBody1() == m_TargetBody)
						normal = -normal; // Fix the normal direction if the target body is the second one in the contact pair

					data.Normal = { normal.x, normal.y, normal.z };

					m_CollisionData.Contacts.push_back(data);
				}
			}
		}

		const CollisionCallbackData& GetCollisionData() const { return m_CollisionData; }

	private:
		CollisionCallbackData m_CollisionData;
		reactphysics3d::Body* m_TargetBody = nullptr;
	};

}