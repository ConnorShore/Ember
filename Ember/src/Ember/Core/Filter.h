#pragma once

#include <cstdint>

using Filter = uint16_t;

namespace FilterPreset
{
	constexpr Filter None = 0x0000;
	constexpr Filter Default = 1ULL;
	constexpr Filter All = 0xFFFF;
}