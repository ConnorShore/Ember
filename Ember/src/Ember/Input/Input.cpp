#include "ebpch.h"
#include "Input.h"
#include "Ember/Core/Core.h"

#define IN_KEY_RANGE(key) key >= 0 && key < KeyArraySize
#define IN_MOUSE_BUTTON_RANGE(button) button >= 0 && button < MouseButtonArraySize

namespace Ember {

	std::array<int, static_cast<size_t>(KeyCode::Last)> Input::s_KeyStates = {};
	std::array<int, static_cast<size_t>(MouseButton::Last)> Input::s_MouseButtonStates = {};
	KEY_MODIFIER_TYPE Input::s_ActiveModifiers = 0;

	bool Input::IsKeyPressed(KeyCode key)
	{
		EB_CORE_ASSERT(IN_KEY_RANGE(key), "Undefined key checked!");
		return s_KeyStates[static_cast<KEY_CODE_TYPE>(key)] > 0;
	}

	bool Input::IsKeyHeld(KeyCode key)
	{
		EB_CORE_ASSERT(IN_KEY_RANGE(key), "Undefined key checked!");
		return s_KeyStates[static_cast<KEY_CODE_TYPE>(key)] > 1;
	}

	bool Input::IsMouseButtonPressed(MouseButton button)
	{
		EB_CORE_ASSERT(IN_MOUSE_BUTTON_RANGE(button), "Undefined mouse button checked!");
		return s_MouseButtonStates[static_cast<MOUSE_BUTTON_TYPE>(button)] > 0;
	}

	bool Input::IsModifierActive(KeyModifier modifier)
	{
		return (s_ActiveModifiers & modifier);
	}

	void Input::SetKeyState(KeyCode key, bool pressed)
	{
		EB_CORE_ASSERT(IN_KEY_RANGE(key), "Undefined key pressed!");
		EB_CORE_TRACE("Key pressed {}", key);
		s_KeyStates[static_cast<KEY_CODE_TYPE>(key)] = 1;
	}

	void Input::IncrementKeyRepeat(KeyCode key)
	{
		EB_CORE_ASSERT(IN_KEY_RANGE(key), "Undefined key repeated!");
		EB_CORE_TRACE("Key {} repeat incremented {}", key, GetKeyRepeatCount(key));
		s_KeyStates[static_cast<KEY_CODE_TYPE>(key)]++;
	}

	void Input::SetMouseButtonState(MouseButton button, bool pressed)
	{
		EB_CORE_ASSERT(IN_MOUSE_BUTTON_RANGE(button), "Undefined mouse button toggled!");
		EB_CORE_TRACE("Mouse button {} state changed. Pressed: {}", button, pressed);
		s_MouseButtonStates[static_cast<MOUSE_BUTTON_TYPE>(button)] = pressed;
	}

	void Input::SetKeyModifierState(KeyModifier modifier, bool active)
	{
		s_ActiveModifiers = active
			? (s_ActiveModifiers | modifier)
			: static_cast<KEY_MODIFIER_TYPE>(s_ActiveModifiers & ~static_cast<KEY_MODIFIER_TYPE>(modifier));
	}

	int Input::GetKeyRepeatCount(KeyCode key)
	{
		EB_CORE_ASSERT(IN_KEY_RANGE(key), "Undefined key checked!");
		return s_KeyStates[static_cast<KEY_CODE_TYPE>(key)];
	}

}