#pragma once
#include "Ember/ECS/Types.h"
#include <reactphysics3d/reactphysics3d.h>
#include <vector>
#include <algorithm>

namespace Ember {

	class TriggerFilterOverlapCallback : public reactphysics3d::OverlapCallback
	{
	public:
		TriggerFilterOverlapCallback(reactphysics3d::Body* targetBody)
			: m_TargetBody(targetBody) {
		}

		virtual void onOverlap(reactphysics3d::OverlapCallback::CallbackData& callbackData) override
		{
			for (uint32_t i = 0; i < callbackData.getNbOverlappingPairs(); i++)
			{
				auto pair = callbackData.getOverlappingPair(i);

				reactphysics3d::Collider* collider1 = pair.getCollider1();
				reactphysics3d::Collider* collider2 = pair.getCollider2();

				// Ignore triggers, we only want solid bodies
				if (collider1->getIsTrigger() || collider2->getIsTrigger())
					continue;

				reactphysics3d::Body* body1 = pair.getBody1();
				reactphysics3d::Body* body2 = pair.getBody2();

				// Filter out the target body
				reactphysics3d::Body* otherBody = (body1 == m_TargetBody) ? body2 : body1;

				// Prevent duplicate entries
				if (std::find(m_SolidBodies.begin(), m_SolidBodies.end(), otherBody) == m_SolidBodies.end())
				{
					m_SolidBodies.push_back(otherBody);
				}
			}
		}

		const std::vector<reactphysics3d::Body*>& GetSolidBodies() const { return m_SolidBodies; }

	private:
		reactphysics3d::Body* m_TargetBody;
		std::vector<reactphysics3d::Body*> m_SolidBodies;
	};
}