#pragma once

#include "InputDevice.h"
#include "InputCode.h"

#include <variant>

namespace Ember {

	struct InputTrigger
	{
		InputDevice Device = InputDevice::None;
		std::variant<KeyCode, MouseControl> ControlId = KeyCode::Unknown;
		KeyModifierType RequiredModifiers = 0;

		// uint16_t operator for controlId
		operator uint16_t() const
		{
			if (std::holds_alternative<KeyCode>(ControlId))
				return static_cast<uint16_t>(std::get<KeyCode>(ControlId));
			else if (std::holds_alternative<MouseControl>(ControlId))
				return static_cast<uint16_t>(std::get<MouseControl>(ControlId));
			else
				return 0;
		}
	};

}