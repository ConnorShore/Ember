#pragma once

#include <cstdint>

namespace Ember {

	using CollisionFilter = uint16_t;

	namespace CollisionFilterPreset
	{
		constexpr CollisionFilter Default =		0ULL;
		constexpr CollisionFilter Environment =	1ULL;
		constexpr CollisionFilter Player = 1ULL << 1;
		constexpr CollisionFilter Enemy = 1ULL << 2;

		constexpr CollisionFilter All = 0xFFFF;
	}
}