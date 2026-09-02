#pragma once

#include <cstdint>

namespace Ember {

	// Which groups of screen-space UI a viewport draws, relative to the editor's selection. Each bit
	// owns a disjoint slice of the UI so the editor can author one canvas without the others on top.
	enum class ScreenSpaceRenderMode : uint32_t
	{
		None			= 0,
		Selected		= 1 << 0,	// the selected entity itself
		Children		= 1 << 1,	// everything parented below the selection
		Parents			= 1 << 2,	// the selection's ancestors, up to but not including its canvas
		Canvas			= 1 << 3,	// the remainder of the selection's canvas
		OtherCanvases	= 1 << 4,	// every canvas the selection does not belong to

		All = Selected | Children | Parents | Canvas | OtherCanvases
	};

	constexpr ScreenSpaceRenderMode operator|(ScreenSpaceRenderMode a, ScreenSpaceRenderMode b)
	{
		return static_cast<ScreenSpaceRenderMode>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	constexpr ScreenSpaceRenderMode operator&(ScreenSpaceRenderMode a, ScreenSpaceRenderMode b)
	{
		return static_cast<ScreenSpaceRenderMode>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	// Masked to All so clearing a bit can never leave one set that no checkbox is able to reach.
	constexpr ScreenSpaceRenderMode operator~(ScreenSpaceRenderMode mode)
	{
		return static_cast<ScreenSpaceRenderMode>(~static_cast<uint32_t>(mode) & static_cast<uint32_t>(ScreenSpaceRenderMode::All));
	}

	constexpr ScreenSpaceRenderMode& operator|=(ScreenSpaceRenderMode& a, ScreenSpaceRenderMode b) { return a = a | b; }
	constexpr ScreenSpaceRenderMode& operator&=(ScreenSpaceRenderMode& a, ScreenSpaceRenderMode b) { return a = a & b; }

	constexpr bool HasFlag(ScreenSpaceRenderMode mode, ScreenSpaceRenderMode flag)
	{
		return (mode & flag) == flag;
	}

}
