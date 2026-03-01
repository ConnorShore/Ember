#pragma once

#include <cstdint>
#include <concepts>
#include <format>

namespace Ember {

#define MOUSE_BUTTON_TYPE uint8_t

	enum class MouseButton : MOUSE_BUTTON_TYPE
	{
		Left = 0,
		Right = 1,
		Middle = 2,
		Last = 3,
		Unknown = 255
	};

	// Forward direction: MouseButton first, integral type second
	template<std::integral T>
	inline bool operator<(MouseButton a, T b) { return static_cast<T>(a) < b; }

	template<std::integral T>
	inline bool operator>(MouseButton a, T b) { return static_cast<T>(a) > b; }

	template<std::integral T>
	inline bool operator<=(MouseButton a, T b) { return static_cast<T>(a) <= b; }

	template<std::integral T>
	inline bool operator>=(MouseButton a, T b) { return static_cast<T>(a) >= b; }

	// Reverse direction: integral type first, MouseButton second
	template<std::integral T>
	inline bool operator<(T a, MouseButton b) { return a < static_cast<T>(b); }

	template<std::integral T>
	inline bool operator>(T a, MouseButton b) { return a > static_cast<T>(b); }

	template<std::integral T>
	inline bool operator<=(T a, MouseButton b) { return a <= static_cast<T>(b); }

	template<std::integral T>
	inline bool operator>=(T a, MouseButton b) { return a >= static_cast<T>(b); }

}

template <>
struct std::formatter<Ember::MouseButton> : std::formatter<int>
{
	auto format(Ember::MouseButton mouseButton, std::format_context& ctx) const
	{
		return std::formatter<int>::format(static_cast<int>(mouseButton), ctx);
	}
};