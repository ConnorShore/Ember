#pragma once

#include <cstdint>
#include <concepts>
#include <format>

namespace Ember {

	using KeyCodeType     = uint16_t;
	using KeyActionType   = uint8_t;
	using KeyModifierType = uint8_t;


	// -------------------------------------------------------------------------------------
	// -------- KeyCode --------------------------------------------------------------------
	// -------------------------------------------------------------------------------------
	// The key list lives in KeyCodes.inl so the enum, the name tables and the Lua bindings all read
	// from one place; adding a key means editing that file only.
	enum class KeyCode : KeyCodeType
	{
		Unknown = 0,

#define EB_KEY(name, value) name = value,
#include "KeyCodes.inl"
#undef EB_KEY

		/* Last Identifier for array sizing */
		Last = 349
	};

	// Forward direction: KeyCode first, integral type second
	template<std::integral T>
	inline bool operator<(KeyCode a, T b) { return static_cast<T>(a) < b; }

	template<std::integral T>
	inline bool operator>(KeyCode a, T b) { return static_cast<T>(a) > b; }

	template<std::integral T>
	inline bool operator<=(KeyCode a, T b) { return static_cast<T>(a) <= b; }

	template<std::integral T>
	inline bool operator>=(KeyCode a, T b) { return static_cast<T>(a) >= b; }

	// Reverse direction: integral type first, KeyCode second
	template<std::integral T>
	inline bool operator<(T a, KeyCode b) { return a < static_cast<T>(b); }

	template<std::integral T>
	inline bool operator>(T a, KeyCode b) { return a > static_cast<T>(b); }

	template<std::integral T>
	inline bool operator<=(T a, KeyCode b) { return a <= static_cast<T>(b); }

	template<std::integral T>
	inline bool operator>=(T a, KeyCode b) { return a >= static_cast<T>(b); }

	// -------------------------------------------------------------------------------------
	// -------- KeyAction ------------------------------------------------------------------
	// -------------------------------------------------------------------------------------
	enum class KeyAction : KeyActionType
	{
		Release = 0,
		Press = 1,
		Repeat = 2
	};

	// -------------------------------------------------------------------------------------
	// -------- KeyModifier ----------------------------------------------------------------
	// -------------------------------------------------------------------------------------
	enum class KeyModifier : KeyModifierType
	{
		None = 0,
		Shift = 1 << 0,
		Control = 1 << 1,
		Alt = 1 << 2,
		Super = 1 << 3
	};

	inline KeyModifierType operator|(KeyModifierType a, KeyModifier b) { return a | static_cast<KeyModifierType>(b); }
	inline KeyModifierType operator&(KeyModifierType a, KeyModifier b) { return a & static_cast<KeyModifierType>(b); }
	inline KeyModifierType& operator|=(KeyModifierType& a, KeyModifier b) { a = a | static_cast<KeyModifierType>(b); return a; }
	inline KeyModifierType& operator&=(KeyModifierType& a, KeyModifier b) { a = a & static_cast<KeyModifierType>(b); return a; }
}

template<>
struct std::formatter<Ember::KeyCode> : std::formatter<int>
{
	auto format(Ember::KeyCode keyCode, std::format_context& ctx) const
	{
		return std::formatter<int>::format(static_cast<int>(keyCode), ctx);
	}
};