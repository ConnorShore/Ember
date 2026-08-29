#include "ebpch.h"
#include "InputCodeNames.h"

#include <utility>

namespace Ember {

	namespace {

		// Serialized key names, generated from the one list in KeyCodes.inl so they cannot drift
		// from the enum. Unknown and Last are not in that list and are handled by the callers.
		constexpr std::pair<std::string_view, KeyCode> s_KeyNames[] =
		{
#define EB_KEY(name, value) { #name, KeyCode::name },
#include "KeyCodes.inl"
#undef EB_KEY
		};

		// Only the keys whose enumerator name reads badly in a rebind menu; everything else falls
		// through to the serialized name.
		constexpr std::pair<KeyCode, std::string_view> s_KeyDisplayOverrides[] =
		{
			{ KeyCode::D0, "0" }, { KeyCode::D1, "1" }, { KeyCode::D2, "2" }, { KeyCode::D3, "3" },
			{ KeyCode::D4, "4" }, { KeyCode::D5, "5" }, { KeyCode::D6, "6" }, { KeyCode::D7, "7" },
			{ KeyCode::D8, "8" }, { KeyCode::D9, "9" },

			{ KeyCode::Apostrophe, "'" },  { KeyCode::Comma, "," },       { KeyCode::Minus, "-" },
			{ KeyCode::Period, "." },      { KeyCode::Slash, "/" },       { KeyCode::Semicolon, ";" },
			{ KeyCode::Equal, "=" },       { KeyCode::LeftBracket, "[" }, { KeyCode::Backslash, "\\" },
			{ KeyCode::RightBracket, "]" },{ KeyCode::GraveAccent, "`" },

			{ KeyCode::PageUp, "Page Up" },         { KeyCode::PageDown, "Page Down" },
			{ KeyCode::CapsLock, "Caps Lock" },     { KeyCode::ScrollLock, "Scroll Lock" },
			{ KeyCode::NumLock, "Num Lock" },       { KeyCode::PrintScreen, "Print Screen" },

			{ KeyCode::LeftShift, "Left Shift" },   { KeyCode::RightShift, "Right Shift" },
			{ KeyCode::LeftControl, "Left Ctrl" },  { KeyCode::RightControl, "Right Ctrl" },
			{ KeyCode::LeftAlt, "Left Alt" },       { KeyCode::RightAlt, "Right Alt" },
			{ KeyCode::LeftSuper, "Left Super" },   { KeyCode::RightSuper, "Right Super" },

			{ KeyCode::NumPad0, "Numpad 0" }, { KeyCode::NumPad1, "Numpad 1" },
			{ KeyCode::NumPad2, "Numpad 2" }, { KeyCode::NumPad3, "Numpad 3" },
			{ KeyCode::NumPad4, "Numpad 4" }, { KeyCode::NumPad5, "Numpad 5" },
			{ KeyCode::NumPad6, "Numpad 6" }, { KeyCode::NumPad7, "Numpad 7" },
			{ KeyCode::NumPad8, "Numpad 8" }, { KeyCode::NumPad9, "Numpad 9" },
			{ KeyCode::NumPadDecimal, "Numpad ." },  { KeyCode::NumPadDivide, "Numpad /" },
			{ KeyCode::NumPadMultiply, "Numpad *" }, { KeyCode::NumPadSubtract, "Numpad -" },
			{ KeyCode::NumPadAdd, "Numpad +" },      { KeyCode::NumPadEnter, "Numpad Enter" },
			{ KeyCode::NumPadEqual, "Numpad =" },
		};

		// Serialized mouse control names, generated from the one list in MouseControls.inl.
		constexpr std::pair<std::string_view, MouseControl> s_MouseNames[] =
		{
#define EB_MOUSE_CONTROL(name, value) { #name, MouseControl::name },
#include "MouseControls.inl"
#undef EB_MOUSE_CONTROL
		};

		constexpr std::pair<MouseControl, std::string_view> s_MouseDisplayOverrides[] =
		{
			{ MouseControl::Left,      "Left Mouse" },
			{ MouseControl::Right,     "Right Mouse" },
			{ MouseControl::Middle,    "Middle Mouse" },
			{ MouseControl::WheelUp,   "Wheel Up" },
			{ MouseControl::WheelDown, "Wheel Down" },
		};

		constexpr std::string_view DeviceKeyboard = "Key";
		constexpr std::string_view DeviceMouse    = "Mouse";
		constexpr std::string_view DeviceGamepad  = "Gamepad";

		// Emitted in this fixed order so a trigger string round-trips to itself.
		constexpr std::pair<std::string_view, KeyModifier> s_ModifierNames[] =
		{
			{ "Ctrl",  KeyModifier::Control },
			{ "Shift", KeyModifier::Shift },
			{ "Alt",   KeyModifier::Alt },
			{ "Super", KeyModifier::Super },
		};

		std::string_view DeviceToString(InputDevice device)
		{
			switch (device)
			{
			case InputDevice::Keyboard: return DeviceKeyboard;
			case InputDevice::Mouse:    return DeviceMouse;
			case InputDevice::Gamepad:  return DeviceGamepad;
			default:                    return {};
			}
		}

		// Returns false when the token is not a modifier, which is how the parser knows to stop
		// splitting on '+' rather than eating the trailing '+' of a name like "LeftStickX+".
		bool ModifierFromString(std::string_view str, KeyModifier& out)
		{
			for (const auto& [name, value] : s_ModifierNames)
			{
				if (name == str)
				{
					out = value;
					return true;
				}
			}

			return false;
		}

		// Appends "Ctrl+Shift+" style prefixes; separator differs between serialized and display form.
		void AppendModifiers(std::string& target, KeyModifierType modifiers, std::string_view separator)
		{
			for (const auto& [name, value] : s_ModifierNames)
			{
				if (modifiers & value)
				{
					target += name;
					target += separator;
				}
			}
		}

	}

	std::string_view InputCodeNames::KeyCodeToString(KeyCode key)
	{
		switch (key)
		{
#define EB_KEY(name, value) case KeyCode::name: return #name;
#include "KeyCodes.inl"
#undef EB_KEY
		default: return "Unknown";
		}
	}

	KeyCode InputCodeNames::KeyCodeFromString(std::string_view str)
	{
		for (const auto& [name, key] : s_KeyNames)
		{
			if (name == str)
				return key;
		}

		return KeyCode::Unknown;
	}

	std::string_view InputCodeNames::KeyCodeDisplayName(KeyCode key)
	{
		for (const auto& [value, display] : s_KeyDisplayOverrides)
		{
			if (value == key)
				return display;
		}

		return KeyCodeToString(key);
	}

	std::string_view InputCodeNames::MouseControlToString(MouseControl control)
	{
		for (const auto& [name, value] : s_MouseNames)
		{
			if (value == control)
				return name;
		}

		return {};
	}

	bool InputCodeNames::MouseControlFromString(std::string_view str, MouseControl& out)
	{
		for (const auto& [name, value] : s_MouseNames)
		{
			if (name == str)
			{
				out = value;
				return true;
			}
		}

		return false;
	}

	std::string_view InputCodeNames::MouseControlDisplayName(MouseControl control)
	{
		for (const auto& [value, display] : s_MouseDisplayOverrides)
		{
			if (value == control)
				return display;
		}

		return MouseControlToString(control);
	}

	std::string InputCodeNames::TriggerToString(const InputTrigger& trigger)
	{
		const std::string_view device = DeviceToString(trigger.Device);
		if (device.empty())
			return {};

		std::string result(device);
		result += '/';

		AppendModifiers(result, trigger.RequiredModifier, "+");

		switch (trigger.Device)
		{
		case InputDevice::Keyboard:
			if (const KeyCode* key = std::get_if<KeyCode>(&trigger.ControlId))
			{
				result += KeyCodeToString(*key);
				return result;
			}
			return {};

		case InputDevice::Mouse:
			if (const MouseControl* control = std::get_if<MouseControl>(&trigger.ControlId))
			{
				const std::string_view name = MouseControlToString(*control);
				if (name.empty())
					return {};

				result += name;
				return result;
			}
			return {};

		default:
			// Gamepad has no control type in InputTrigger::ControlId yet.
			return {};
		}
	}

	bool InputCodeNames::TriggerFromString(std::string_view str, InputTrigger& out)
	{
		out = InputTrigger{};

		const size_t slash = str.find('/');
		if (slash == std::string_view::npos)
			return false;

		const std::string_view device = str.substr(0, slash);
		std::string_view remainder = str.substr(slash + 1);

		// Modifier prefixes, accepted in any order even though we always emit one.
		KeyModifierType modifiers = 0;
		while (true)
		{
			const size_t plus = remainder.find('+');
			if (plus == std::string_view::npos)
				break;

			KeyModifier modifier{};
			if (!ModifierFromString(remainder.substr(0, plus), modifier))
				break;

			modifiers |= modifier;
			remainder.remove_prefix(plus + 1);
		}

		if (remainder.empty())
			return false;

		if (device == DeviceKeyboard)
		{
			const KeyCode key = KeyCodeFromString(remainder);
			if (key == KeyCode::Unknown)
				return false;

			out.Device = InputDevice::Keyboard;
			out.ControlId = key;
		}
		else if (device == DeviceMouse)
		{
			MouseControl control{};
			if (!MouseControlFromString(remainder, control))
				return false;

			out.Device = InputDevice::Mouse;
			out.ControlId = control;
		}
		else
		{
			// Unknown device, or Gamepad before it is implemented.
			return false;
		}

		out.RequiredModifier = modifiers;
		return true;
	}

	std::string InputCodeNames::TriggerToDisplayName(const InputTrigger& trigger)
	{
		std::string result;
		AppendModifiers(result, trigger.RequiredModifier, " + ");

		switch (trigger.Device)
		{
		case InputDevice::Keyboard:
			if (const KeyCode* key = std::get_if<KeyCode>(&trigger.ControlId))
			{
				result += KeyCodeDisplayName(*key);
				return result;
			}
			break;

		case InputDevice::Mouse:
			if (const MouseControl* control = std::get_if<MouseControl>(&trigger.ControlId))
			{
				result += MouseControlDisplayName(*control);
				return result;
			}
			break;

		default:
			break;
		}

		return "Unbound";
	}

}
