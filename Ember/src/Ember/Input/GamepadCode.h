#pragma once

#include <cstdint>
#include <format>

namespace Ember {

	using GamepadButtonType = uint8_t;
	using GamepadAxisType = uint8_t;

	enum class GamepadButton : GamepadButtonType
	{
#define EB_GAMEPAD_BUTTON(name, value) name = value,
#include "GamepadButton.inl"
#undef EB_GAMEPAD_BUTTON

		Last = 15
	};

	enum class GamepadAxis : GamepadAxisType
	{
#define EB_GAMEPAD_AXIS(name, value) name = value,
#include "GamepadAxis.inl"
#undef EB_GAMEPAD_AXIS
		Last = 6
	};

	// Names come straight from the .inl tables so they cannot drift from the enum values.
	inline const char* GamepadButtonToString(GamepadButton button)
	{
		switch (button)
		{
#define EB_GAMEPAD_BUTTON(name, value) case GamepadButton::name: return #name;
#include "GamepadButton.inl"
#undef EB_GAMEPAD_BUTTON
			default: return "Unknown";
		}
	}

	inline const char* GamepadAxisToString(GamepadAxis axis)
	{
		switch (axis)
		{
#define EB_GAMEPAD_AXIS(name, value) case GamepadAxis::name: return #name;
#include "GamepadAxis.inl"
#undef EB_GAMEPAD_AXIS
			default: return "Unknown";
		}
	}
}

template <>
struct std::formatter<Ember::GamepadButton> : std::formatter<int>
{
	auto format(Ember::GamepadButton gamepadButton, std::format_context& ctx) const
	{
		return std::formatter<int>::format(static_cast<int>(gamepadButton), ctx);
	}
};

template <>
struct std::formatter<Ember::GamepadAxis> : std::formatter<int>
{
	auto format(Ember::GamepadAxis gamepadAxis, std::format_context& ctx) const
	{
		return std::formatter<int>::format(static_cast<int>(gamepadAxis), ctx);
	}
};