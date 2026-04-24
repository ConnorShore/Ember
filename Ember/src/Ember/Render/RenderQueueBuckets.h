#pragma once

#include "Ember/ECS/Types.h"

#include <vector>

namespace Ember {

	struct RenderQueueBuckets
	{
		std::vector<EntityID> Opaque;
		std::vector<EntityID> Forward;
		std::vector<EntityID> Transparent;

		void Clear()
		{
			Opaque.clear();
			Forward.clear();
			Transparent.clear();
		}
	};

}