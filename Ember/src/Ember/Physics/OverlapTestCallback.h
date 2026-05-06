#pragma once

#include "ColliderUserData.h"
#include "Ember/ECS/Types.h"

#include <reactphysics3d/collision/OverlapCallback.h>

namespace Ember {

	struct Hit
	{
		EntityID EntityID = Constants::Entities::InvalidEntityID;
		CollisionFilter Filter = CollisionFilterPreset::Default;
	};

	struct OverlapTestData
	{
		
		//bool HasHit = false;
		//uint32_t NumCollisions = 0;

		//std::vector<EntityID> CollidedEntities;
		//std::vector<CollisionFilter> CollidedEntityFilters;
		std::vector<Hit> Hits;

		operator bool() const { return !Hits.empty(); }
	};

	class OverlapTestCallback : public reactphysics3d::OverlapCallback
	{
	public:
		OverlapTestCallback(reactphysics3d::RigidBody* bodyToIgnore = nullptr, reactphysics3d::RigidBody* probeBody = nullptr)
			: m_BodyToIgnore(bodyToIgnore), m_ProbeBody(probeBody) {}

		virtual void onOverlap(reactphysics3d::OverlapCallback::CallbackData& callbackData) override
		{
			//uint32_t numCollisions = 0;
			for (uint32_t i = 0; i < callbackData.getNbOverlappingPairs(); i++)
			{
				auto pair = callbackData.getOverlappingPair(i);
				reactphysics3d::RigidBody* body1 = static_cast<reactphysics3d::RigidBody*>(pair.getBody1());
				reactphysics3d::RigidBody* body2 = static_cast<reactphysics3d::RigidBody*>(pair.getBody2());

				// Determine which body is the collided entity (not the probe and not the ignored body)
				reactphysics3d::RigidBody* collidedBody = nullptr;
				if (body1 == m_ProbeBody || body1 == m_BodyToIgnore)
					collidedBody = body2;
				else if (body2 == m_ProbeBody || body2 == m_BodyToIgnore)
					collidedBody = body1;

				if (collidedBody == nullptr || collidedBody == m_ProbeBody || collidedBody == m_BodyToIgnore)
					continue;

				// User data is stored on the collider, not the body - grab it from the first collider
				if (collidedBody->getNbColliders() == 0)
					continue;

				ColliderUserData* collisionData = static_cast<ColliderUserData*>(collidedBody->getCollider(0)->getUserData());
				if (collisionData == nullptr)
					continue;

				m_OverlapData.Hits.push_back({ collisionData->EntityID, collisionData->Filter });

				//numCollisions++;
				//m_OverlapData.HasHit = true;
				//m_OverlapData.CollidedEntities.push_back(collisionData->EntityID);
				//m_OverlapData.CollidedEntityFilters.push_back(collisionData->Filter);
			}
			//m_OverlapData.NumCollisions = numCollisions;
		}

		const OverlapTestData& GetOverlapData() const { return m_OverlapData; }

	private:
		reactphysics3d::RigidBody* m_BodyToIgnore = nullptr;
		reactphysics3d::RigidBody* m_ProbeBody = nullptr;
		OverlapTestData m_OverlapData;
	};
}