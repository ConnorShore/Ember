#pragma once

#include <array>
#include <cstdint>

namespace Ember {

	// Per-stick rather than global because an FPS inverts the look stick alone, while UIInputSystem
	// navigates menus with the left one.
	enum class GamepadStick : uint8_t
	{
		Left = 0,
		Right,
		Last
	};

	enum class GamepadTrigger : uint8_t
	{
		Left = 0,
		Right,
		Last
	};

	// Every default is neutral, so an untouched project reads exactly as it did before conditioning.
	struct StickSettings
	{
		float Deadzone = 0.15f;    // inner radial deadzone, as a fraction of full deflection
		float Saturation = 1.0f;   // deflection that already counts as fully pushed
		float Exponent = 1.0f;     // 1 = linear, >1 = finer control near centre
		float Actuation = 0.5f;    // physical travel before a digital read calls it pressed
		bool InvertX = false;
		bool InvertY = false;
	};

	struct TriggerSettings
	{
		float Deadzone = 0.05f;
		float Saturation = 1.0f;
		float Exponent = 1.0f;
		float Actuation = 0.15f;
	};

	// No sensitivity: the mouse delta has several readers and only each one knows its own scale.
	struct MouseSettings
	{
		bool InvertX = false;
		bool InvertY = false;
	};

	struct InputSettings
	{
		std::array<StickSettings, static_cast<size_t>(GamepadStick::Last)> Sticks;
		std::array<TriggerSettings, static_cast<size_t>(GamepadTrigger::Last)> Triggers;
		MouseSettings Mouse;
	};

}
