#include "ebpch.h"
#include "Input.h"

#include "Ember/Core/Core.h"
#include "Ember/Core/Application.h"

#define IN_KEY_RANGE(key) key >= 0 && key < KeyArraySize
#define IN_MOUSE_BUTTON_RANGE(button) button >= 0 && button < MouseButtonArraySize

namespace Ember {

	std::array<int, static_cast<size_t>(KeyCode::Last)> Input::s_KeyStates = {};
	std::array<int, static_cast<size_t>(MouseButton::Last)> Input::s_MouseButtonStates = {};
	KeyModifierType Input::s_ActiveModifiers = 0;
	Vector2f Input::s_MousePosition = { 0.0f, 0.0f };
	Vector2f Input::s_ScrollOffset = { 0.0f, 0.0f };
	Vector2f Input::s_PreviousMousePosition = { 0.0f, 0.0f };

	Vector2f Input::s_ViewportMin = { 0.0f, 0.0f };
	Vector2f Input::s_ViewportSize = { 0.0f, 0.0f };
	bool Input::s_ViewportInputActive = true;

	bool Input::IsKeyPressed(KeyCode key)
	{
		EB_CORE_ASSERT(IN_KEY_RANGE(key), "Undefined key checked!");
		return s_KeyStates[static_cast<KeyCodeType>(key)] > 0;
	}

	bool Input::IsKeyHeld(KeyCode key)
	{
		EB_CORE_ASSERT(IN_KEY_RANGE(key), "Undefined key checked!");
		// State > 1 means the key has been pressed AND repeated at least once
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

		// Only count repeats for keys we have actually seen pressed. GLFW does not synthesize a PRESS for
		// keys already held when the window gains focus, so stray repeats would otherwise leave
		// IsKeyPressed() stuck true until the user tapped the key.
		auto& state = s_KeyStates[static_cast<KeyCodeType>(key)];
		if (state > 0)
			state++;
	}

	void Input::SetMouseButtonState(MouseButton button, bool pressed)
	{
		EB_CORE_ASSERT(IN_MOUSE_BUTTON_RANGE(button), "Undefined mouse button toggled!");
		s_MouseButtonStates[static_cast<MouseButtonType>(button)] = pressed;
	}

	void Input::ClearAllStates()
	{
		s_KeyStates.fill(0);
		s_MouseButtonStates.fill(0);
		s_ActiveModifiers = 0;
	}

	void Input::SetKeyModifierState(KeyModifier modifier, bool active)
	{
		// Set or clear the modifier bit using bitwise OR / AND-NOT
		s_ActiveModifiers = active
			? (s_ActiveModifiers | modifier)
			: static_cast<KeyModifierType>(s_ActiveModifiers & ~static_cast<KeyModifierType>(modifier));
	}

	void Input::SetMousePosition(const Vector2f& position)
	{
		Application::Instance().SetCursorPosition(position.x, position.y);
		UpdateMousePosition(position);
		ResetMouseDelta();
	}

	void Input::UpdateMousePosition(const Vector2f& position)
	{
		s_PreviousMousePosition = s_MousePosition;
		s_MousePosition = position;
	}

	void Input::SetMouseScrollOffset(const Vector2f& offset)
	{
		s_ScrollOffset = offset;
	}

	void Input::SetCursorMode(CursorMode mode)
	{
		Application::Instance().SetCursorMode(mode);
	}

	CursorMode Input::GetCursorMode()
	{
		return Application::Instance().GetCursorMode();
	}

	const Vector2f& Input::GetMousePosition()
	{
		return s_MousePosition;
	}

	const Vector2f& Input::GetMouseScrollOffset()
	{
		return s_ScrollOffset;
	}

	Vector2f Input::GetMouseDelta()
	{
		Vector2f ret = s_MousePosition - s_PreviousMousePosition;

		if (std::abs(ret.x) < Input::Deadzone) 
			ret.x = 0.0f;
		if (std::abs(ret.y) < Input::Deadzone) 
			ret.y = 0.0f;

		return ret;
	}

	void Input::ResetMouseDelta()
	{
		s_PreviousMousePosition = s_MousePosition;
	}

	void Input::SetViewportRect(const Vector2f& min, const Vector2f& size, bool inputActive)
	{
		s_ViewportMin = min;
		s_ViewportSize = size;
		s_ViewportInputActive = inputActive;
	}

	const Vector2f& Input::GetViewportMin()
	{
		return s_ViewportMin;
	}

	const Vector2f& Input::GetViewportSize()
	{
		return s_ViewportSize;
	}

	bool Input::IsViewportInputActive()
	{
		return s_ViewportInputActive;
	}

	bool Input::IsMouseInViewport()
	{
		if (s_ViewportSize.x <= 0.0f || s_ViewportSize.y <= 0.0f)
			return false;

		Vector2f local = s_MousePosition - s_ViewportMin;
		return local.x >= 0.0f && local.y >= 0.0f && local.x < s_ViewportSize.x && local.y < s_ViewportSize.y;
	}

	Vector2f Input::GetViewportMousePosition()
	{
		// Window coords are top-left origin / +Y down; UI space is bottom-left origin / +Y up.
		return Vector2f(s_MousePosition.x - s_ViewportMin.x,
			(s_ViewportMin.y + s_ViewportSize.y) - s_MousePosition.y);
	}

	int Input::GetKeyRepeatCount(KeyCode key)
	{
		EB_CORE_ASSERT(IN_KEY_RANGE(key), "Undefined key checked!");
		return s_KeyStates[static_cast<KeyCodeType>(key)];
	}

}