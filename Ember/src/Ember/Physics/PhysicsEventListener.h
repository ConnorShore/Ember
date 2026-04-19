#pragma once

#include "OverlapTestCallback.h"
#include "OverlapTriggerEventData.h"

#include <reactphysics3d/engine/EventListener.h>

#include <vector>

namespace Ember {

	class PhysicsEventListener : public reactphysics3d::EventListener
	{
	public:

		void onContact(const reactphysics3d::CollisionCallback::CallbackData& data) override
		{
			// TODO: Implement later when a use arises
		}

		void onTrigger(const reactphysics3d::OverlapCallback::CallbackData& callbackData) override
		{
			// Register all overlapping pairs and their event type
			for (uint32_t i = 0; i < callbackData.getNbOverlappingPairs(); i++)
			{
				reactphysics3d::OverlapCallback::OverlapPair pair = callbackData.getOverlappingPair(i);

				OverlapTriggerEventData eventData;
				eventData.EntityA = static_cast<EntityID>(reinterpret_cast<uintptr_t>(pair.getBody1()->getUserData()));
				eventData.EntityB = static_cast<EntityID>(reinterpret_cast<uintptr_t>(pair.getBody2()->getUserData()));

				// Set event type
				switch (pair.getEventType())
				{
				case reactphysics3d::OverlapCallback::OverlapPair::EventType::OverlapStart:
					eventData.EventType = TriggerEventType::OverlapEnter;
					break;
				case reactphysics3d::OverlapCallback::OverlapPair::EventType::OverlapStay:
					eventData.EventType = TriggerEventType::OverlapStay;
					break;
				case reactphysics3d::OverlapCallback::OverlapPair::EventType::OverlapExit:
					eventData.EventType = TriggerEventType::OverlapExit;
					break;
				}

				m_OverlapData.push_back(eventData);
			}
		}

		const std::vector<OverlapTriggerEventData>& GetOverlapData() const { return m_OverlapData; }

		void ClearOverlapQueue() { m_OverlapData.clear(); }

	private:
		std::vector<OverlapTriggerEventData> m_OverlapData;
	};

}