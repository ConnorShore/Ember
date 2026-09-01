#pragma once

#include "InputCode.h"
#include "InputTrigger.h"

#include <string>
#include <string_view>

namespace Ember {

	class InputCodeNames
	{
	public:
		static std::string_view KeyCodeToString(KeyCode key);
		static KeyCode KeyCodeFromString(std::string_view str);

		static std::string_view KeyCodeDisplayName(KeyCode key);

		static std::string_view MouseControlToString(MouseControl control);
		static bool MouseControlFromString(std::string_view str, MouseControl& out);

		static std::string_view MouseControlDisplayName(MouseControl control);

		static std::string_view GamepadButtonToString(GamepadButton button);
		static bool GamepadButtonFromString(std::string_view str, GamepadButton& out);
		
		static std::string_view GamepadButtonDisplayName(GamepadButton button);

		static std::string_view GamepadAxisToString(GamepadAxis axis);
		static bool GamepadAxisFromString(std::string_view str, GamepadAxis& out);

		static std::string_view GamepadAxisDisplayName(GamepadAxis axis);

		// The name of one half of an axis, e.g. "Left Stick Up" - Full falls back to the whole-axis name.
		static std::string_view GamepadAxisDisplayName(GamepadAxis axis, AxisDirection direction);

		static std::string TriggerToString(const InputTrigger& trigger);
		static bool TriggerFromString(std::string_view str, InputTrigger& out);

		static std::string TriggerToDisplayName(const InputTrigger& trigger);
	};

}