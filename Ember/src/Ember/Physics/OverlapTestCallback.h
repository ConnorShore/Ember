#pragma once

#include "Ember/ECS/Types.h"

#include <reactphysics3d/collision/OverlapCallback.h>

namespace Ember {

	struct OverlapTestData
	{
		bool HasHit = false;
		EntityID CollidedEntity = Constants::Entities::InvalidEntityID;
	};

	class OverlapTestCallback : public reactphysics3d::OverlapCallback
	{
	public:
		OverlapTestCallback(reactphysics3d::RigidBody* bodyToIgnore = nullptr, reactphysics3d::RigidBody* probeBody = nullptr)
			: m_BodyToIgnore(bodyToIgnore), m_ProbeBody(probeBody) {}

		virtual void onOverlap(reactphysics3d::OverlapCallback::CallbackData& callbackData) override
		{
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

				m_OverlapData.HasHit = true;
				m_OverlapData.CollidedEntity = static_cast<EntityID>(reinterpret_cast<uintptr_t>(collidedBody->getUserData()));
				return;
			}
		}

		const OverlapTestData& GetOverlapData() const { return m_OverlapData; }

	private:
		reactphysics3d::RigidBody* m_BodyToIgnore = nullptr;
		reactphysics3d::RigidBody* m_ProbeBody = nullptr;
		OverlapTestData m_OverlapData;
	};
}