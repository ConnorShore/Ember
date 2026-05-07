#pragma once

#include <string>

namespace Ember {

	struct AnimationEvent
	{
		std::string Name;
		float Timestamp;

		AnimationEvent() = default;
		AnimationEvent(const std::string& name, float timestamp)
			: Name(name), Timestamp(timestamp) {
		}
	};

}