#pragma once

#include <cstdint>

namespace Ember {

	using CollisionFilter = uint16_t;

	namespace CollisionFilterPreset
	{
		constexpr CollisionFilter Default =		0ULL;      // 0 - means "unset", not a real bit
		constexpr CollisionFilter Environment =	1ULL;      // 1 - slot 0 (0x0001)
		constexpr CollisionFilter Player =		1ULL << 1; // 2 - slot 1 (0x0002)
		constexpr CollisionFilter Enemy =		1ULL << 2; // 4 - slot 2 (0x0004)

		constexpr CollisionFilter All = 0xFFFF;
	}
}