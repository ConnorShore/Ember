#pragma once

#include <reactphysics3d/collision/OverlapCallback.h>

namespace Ember {

	class OverlapTestCallback : public reactphysics3d::OverlapCallback
	{
	public:
		OverlapTestCallback(reactphysics3d::RigidBody* bodyToIgnore = nullptr)
			: m_BodyToIgnore(bodyToIgnore) {}

		virtual void onOverlap(reactphysics3d::OverlapCallback::CallbackData& callbackData) override
		{
			for (uint32_t i = 0; i < callbackData.getNbOverlappingPairs(); i++)
			{
				auto pair = callbackData.getOverlappingPair(i);
				reactphysics3d::RigidBody* body1 = static_cast<reactphysics3d::RigidBody*>(pair.getBody1());
				reactphysics3d::RigidBody* body2 = static_cast<reactphysics3d::RigidBody*>(pair.getBody2());

				if (body1 == m_BodyToIgnore || body2 == m_BodyToIgnore)
					continue;

				m_HasHit = true;
				return;
			}
		}

		const bool HasHit() const { return m_HasHit; }

	private:
		reactphysics3d::RigidBody* m_BodyToIgnore = nullptr;
		bool m_HasHit = false;
	};
}