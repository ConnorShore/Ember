#pragma once

#include <cstdint>

namespace Ember {

	using CollisionFilter = uint16_t;

	namespace CollisionFilterPreset
	{
		constexpr CollisionFilter Default =		0ULL;      // 0 - means "unset", not a real bit
		constexpr CollisionFilter Environment = 1ULL;      
		constexpr CollisionFilter VFX		  = 1ULL << 1; 
		constexpr CollisionFilter Player =		1ULL << 2; 
		constexpr CollisionFilter Enemy =		1ULL << 3; 

		//constexpr CollisionFilter Sensor = 1ULL << 4; // For things like camera sensors, trigger volumes, etc. that should only check against specific categories (e.g. VFX)
		constexpr CollisionFilter All = 0xFFFF;
	}
}