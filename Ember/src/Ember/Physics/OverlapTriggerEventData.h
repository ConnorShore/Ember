#pragma once

#include "Ember/ECS/Types.h"

namespace Ember {

	enum class TriggerEventType
	{
		OverlapEnter,
		OverlapStay,
		OverlapExit
	};

	struct OverlapTriggerEventData
	{
		EntityID EntityA;
		EntityID EntityB;

		TriggerEventType EventType;
	};


}