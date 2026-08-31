#pragma once

#include "InputDevice.h"
#include "InputCode.h"

#include <variant>

namespace Ember {

	// Which half of an axis a trigger listens to. Full is the signed read, and is what every
	// non-axis trigger leaves it at.
	enum class AxisDirection : uint8_t
	{
		Full = 0,
		Positive,
		Negative
	};

	struct InputTrigger
	{
		InputDevice Device = InputDevice::None;
		std::variant<KeyCode, MouseControl, GamepadButton, GamepadAxis> ControlId = KeyCode::Unknown;
		KeyModifierType RequiredModifiers = 0;
		AxisDirection Direction = AxisDirection::Full;

		// uint16_t operator for controlId
		operator uint16_t() const
		{
			if (std::holds_alternative<KeyCode>(ControlId))
				return static_cast<uint16_t>(std::get<KeyCode>(ControlId));
			else if (std::holds_alternative<MouseControl>(ControlId))
				return static_cast<uint16_t>(std::get<MouseControl>(ControlId));
			else if (std::holds_alternative<GamepadButton>(ControlId))
				return static_cast<uint16_t>(std::get<GamepadButton>(ControlId));
			else if (std::holds_alternative<GamepadAxis>(ControlId))
				return static_cast<uint16_t>(std::get<GamepadAxis>(ControlId));
			else
				return 0;
		}
	};

}