#pragma once

#include <cstdint>

namespace Ember {

	using RenderLayer = uint16_t;

	namespace RenderLayerPreset
	{
		constexpr RenderLayer Default = 1ULL;
		constexpr RenderLayer All = 0xFFFF;
	}

}