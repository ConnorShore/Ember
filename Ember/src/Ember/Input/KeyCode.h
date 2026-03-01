#pragma once

#include <cstdint>
#include <concepts>
#include <format>

namespace Ember {

#define KEY_CODE_TYPE uint16_t
#define KEY_ACTION_TYPE uint8_t
#define KEY_MODIFIER_TYPE uint8_t


	// -------------------------------------------------------------------------------------
	// -------- KeyCode --------------------------------------------------------------------
	// -------------------------------------------------------------------------------------
	enum class KeyCode : KEY_CODE_TYPE
	{
		Unknown = 0,
		Space = 32,
		Apostrophe = 39, /* ' */
		Comma = 44, /* , */
		Minus = 45, /* - */
		Period = 46, /* . */
		Slash = 47, /* / */
		D0 = 48, /* 0 */
		D1 = 49, /* 1 */
		D2 = 50, /* 2 */
		D3 = 51, /* 3 */
		D4 = 52, /* 4 */
		D5 = 53, /* 5 */
		D6 = 54, /* 6 */
		D7 = 55, /* 7 */
		D8 = 56, /* 8 */
		D9 = 57, /* 9 */
		Semicolon = 59, /* ; */
		Equal = 61, /* = */
		A = 65,
		B = 66,
		C = 67,
		D = 68,
		E = 69,
		F = 70,
		G = 71,
		H = 72,
		I = 73,
		J = 74,
		K = 75,
		L = 76,
		M = 77,
		N = 78,
		O = 79,
		P = 80,
		Q = 81,
		R = 82,
		S = 83,
		T = 84,
		U = 85,
		V = 86,
		W = 87,
		X = 88,
		Y = 89,
		Z = 90,
		LeftBracket = 91,   /* [ */
		Backslash = 92,     /* \ */
		RightBracket = 93,  /* ] */
		GraveAccent = 96,   /* ` */

		/* Function keys */
		Escape = 256,
		Enter = 257,
		Tab = 258,
		Backspace = 259,
		Insert = 260,
		Delete = 261,
		Right = 262,
		Left = 263,
		Down = 264,
		Up = 265,
		PageUp = 266,
		PageDown = 267,
		Home = 268,
		End = 269,
		CapsLock = 280,
		ScrollLock = 281,
		NumLock = 282,
		PrintScreen = 283,
		Pause = 284,
		F1 = 290,
		F2 = 291,
		F3 = 292,
		F4 = 293,
		F5 = 294,
		F6 = 295,
		F7 = 296,
		F8 = 297,
		F9 = 298,
		F10 = 299,
		F11 = 300,
		F12 = 301,

		/* NumPad */
		NumPad0 = 320,
		NumPad1 = 321,
		NumPad2 = 322,
		NumPad3 = 323,
		NumPad4 = 324,
		NumPad5 = 325,
		NumPad6 = 326,
		NumPad7 = 327,
		NumPad8 = 328,
		NumPad9 = 329,
		NumPadDecimal = 330,
		NumPadDivide = 331,
		NumPadMultiply = 332,
		NumPadSubtract = 333,
		NumPadAdd = 334,
		NumPadEnter = 335,
		NumPadEqual = 336,

		/* Modifier keys */
		LeftShift = 340,
		LeftControl = 341,
		LeftAlt = 342,
		LeftSuper = 343,
		RightShift = 344,
		RightControl = 345,
		RightAlt = 346,
		RightSuper = 347,
		Menu = 348,

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
	enum class KeyAction : KEY_ACTION_TYPE
	{
		Release = 0,
		Press = 1,
		Repeat = 2
	};

	// -------------------------------------------------------------------------------------
	// -------- KeyModifier ----------------------------------------------------------------
	// -------------------------------------------------------------------------------------
	enum class KeyModifier : KEY_MODIFIER_TYPE
	{
		None = 0,
		Shift = 1 << 0,
		Control = 1 << 1,
		Alt = 1 << 2,
		Super = 1 << 3
	};

	inline KEY_MODIFIER_TYPE operator|(KEY_MODIFIER_TYPE a, KeyModifier b) { return a | static_cast<KEY_MODIFIER_TYPE>(b); }
	inline KEY_MODIFIER_TYPE operator&(KEY_MODIFIER_TYPE a, KeyModifier b) { return a & static_cast<KEY_MODIFIER_TYPE>(b); }
}

template<>
struct std::formatter<Ember::KeyCode> : std::formatter<int>
{
	auto format(Ember::KeyCode keyCode, std::format_context& ctx) const
	{
		return std::formatter<int>::format(static_cast<int>(keyCode), ctx);
	}
};