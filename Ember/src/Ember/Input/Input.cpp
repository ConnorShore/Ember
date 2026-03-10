#include "ebpch.h"
#include "Input.h"
#include "Ember/Core/Core.h"

#define IN_KEY_RANGE(key) key >= 0 && key < KeyArraySize
#define IN_MOUSE_BUTTON_RANGE(button) button >= 0 && button < MouseButtonArraySize

namespace Ember {

	std::array<int, static_cast<size_t>(KeyCode::Last)> Input::s_KeyStates = {};
	std::array<int, static_cast<size_t>(MouseButton::Last)> Input::s_MouseButtonStates = {};
	KeyModifierType Input::s_ActiveModifiers = 0;

	bool Input::IsKeyPressed(KeyCode key)
	{
		EB_CORE_ASSERT(IN_KEY_RANGE(key), "Undefined key checked!");
		return s_KeyStates[static_cast<KeyCodeType>(key)] > 0;
	}

	bool Input::IsKeyHeld(KeyCode key)
	{
		EB_CORE_ASSERT(IN_KEY_RANGE(key), "Undefined key checked!");
		return s_KeyStates[static_cast<KeyCodeType>(key)] > 1;
	}

	bool Input::IsMouseButtonPressed(MouseButton button)
	{
		EB_CORE_ASSERT(IN_MOUSE_BUTTON_RANGE(button), "Undefined mouse button checked!");
		return s_MouseButtonStates[static_cast<MouseButtonType>(button)] > 0;
	}

	bool Input::IsModifierActive(KeyModifier modifier)
	{
		return (s_ActiveModifiers & modifier);
	}

	void Input::SetKeyState(KeyCode key, bool pressed)
	{
		EB_CORE_ASSERT(IN_KEY_RANGE(key), "Undefined key pressed!");
		s_KeyStates[static_cast<KeyCodeType>(key)] = pressed ? 1 : 0;
	}

	void Input::IncrementKeyRepeat(KeyCode key)
	{
		EB_CORE_ASSERT(IN_KEY_RANGE(key), "Undefined key repeated!");
		s_KeyStates[static_cast<KeyCodeType>(key)]++;
	}

	void Input::SetMouseButtonState(MouseButton button, bool pressed)
	{
		EB_CORE_ASSERT(IN_MOUSE_BUTTON_RANGE(button), "Undefined mouse button toggled!");
		s_MouseButtonStates[static_cast<MouseButtonType>(button)] = pressed;
	}

	void Input::SetKeyModifierState(KeyModifier modifier, bool active)
	{
		s_ActiveModifiers = active
			? (s_ActiveModifiers | modifier)
			: static_cast<KeyModifierType>(s_ActiveModifiers & ~static_cast<KeyModifierType>(modifier));
	}

	int Input::GetKeyRepeatCount(KeyCode key)
	{
		EB_CORE_ASSERT(IN_KEY_RANGE(key), "Undefined key checked!");
		return s_KeyStates[static_cast<KeyCodeType>(key)];
	}

}