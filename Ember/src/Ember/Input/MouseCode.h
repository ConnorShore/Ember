#pragma once

#include <cstdint>
#include <concepts>
#include <format>

namespace Ember {

	using MouseButtonType = uint8_t;

	enum class MouseButton : MouseButtonType
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

	// The control list lives in MouseControls.inl so the enum and the name tables read from one place.
	enum class MouseControl : uint16_t
	{
#define EB_MOUSE_CONTROL(name, value) name = value,
#include "MouseControls.inl"
#undef EB_MOUSE_CONTROL

		/* Last identifier for array sizing; not a control, so it stays out of the list above. */
		Last = 18
	};

}

template <>
struct std::formatter<Ember::MouseButton> : std::formatter<int>
{
	auto format(Ember::MouseButton mouseButton, std::format_context& ctx) const
	{
		return std::formatter<int>::format(static_cast<int>(mouseButton), ctx);
	}
};