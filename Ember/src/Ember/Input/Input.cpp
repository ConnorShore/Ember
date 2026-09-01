#include "ebpch.h"
#include "Input.h"

#include "Ember/Core/Core.h"
#include "Ember/Core/Application.h"

#define IN_KEY_RANGE(key) key >= 0 && key < KeyArraySize
#define IN_MOUSE_CONTROL_RANGE(control) static_cast<size_t>(control) < MouseControlArraySize

namespace Ember {

	std::array<int, static_cast<size_t>(KeyCode::Last)> Input::s_KeyStates = {};
	std::array<uint8_t, static_cast<size_t>(KeyCode::Last)> Input::s_KeyDownSnapshot = {};
	std::array<uint8_t, static_cast<size_t>(KeyCode::Last)> Input::s_KeyPressedSnapshot = {};
	std::array<uint8_t, static_cast<size_t>(KeyCode::Last)> Input::s_KeyReleasedSnapshot = {};
	std::array<uint8_t, static_cast<size_t>(KeyCode::Last)> Input::s_KeyPressLatch = {};
	std::array<uint8_t, static_cast<size_t>(KeyCode::Last)> Input::s_KeyReleaseLatch = {};

	std::array<int, static_cast<size_t>(MouseControl::Last)> Input::s_MouseControlStates = {};
	std::array<uint8_t, static_cast<size_t>(MouseControl::Last)> Input::s_MouseControlDownSnapshot = {};
	std::array<uint8_t, static_cast<size_t>(MouseControl::Last)> Input::s_MouseControlPressedSnapshot = {};
	std::array<uint8_t, static_cast<size_t>(MouseControl::Last)> Input::s_MouseControlReleasedSnapshot = {};
	std::array<uint8_t, static_cast<size_t>(MouseControl::Last)> Input::s_MouseControlPressLatch = {};
	std::array<uint8_t, static_cast<size_t>(MouseControl::Last)> Input::s_MouseControlReleaseLatch = {};

	std::array<Ember::GamepadState, Ember::Input::MaxGamepads> Input::s_GamepadStates = {};

	KeyModifierType Input::s_ActiveModifiers = 0;
	Vector2f Input::s_MousePosition = { 0.0f, 0.0f };
	Vector2f Input::s_ScrollOffset = { 0.0f, 0.0f };
	Vector2f Input::s_PreviousMousePosition = { 0.0f, 0.0f };

	Vector2f Input::s_ViewportMin = { 0.0f, 0.0f };
	Vector2f Input::s_ViewportSize = { 0.0f, 0.0f };

	bool Input::s_ViewportInputActive = true;

	InputDevice Input::s_LastUsedDevice = InputDevice::None;

	void Input::BeginFrame()
	{
		// Update snapshots of key states
		for (size_t k = 0; k < KeyArraySize; k++)
		{
			bool down = s_KeyStates[k] > 0, prev = s_KeyDownSnapshot[k];
			s_KeyPressedSnapshot[k] = (down && !prev) || s_KeyPressLatch[k] > 0;
			s_KeyReleasedSnapshot[k] = (!down && prev) || s_KeyReleaseLatch[k] > 0;
			s_KeyDownSnapshot[k] = down;
			s_KeyPressLatch[k] = s_KeyReleaseLatch[k] = 0;
		}

		// Update snapshots of mouse button states
		for (size_t b = 0; b < MouseControlArraySize; b++)
		{
			bool down = s_MouseControlStates[b] > 0, prev = s_MouseControlDownSnapshot[b];
			s_MouseControlPressedSnapshot[b] = (down && !prev) || s_MouseControlPressLatch[b] > 0;
			s_MouseControlReleasedSnapshot[b] = (!down && prev) || s_MouseControlReleaseLatch[b] > 0;
			s_MouseControlDownSnapshot[b] = down;
			s_MouseControlPressLatch[b] = s_MouseControlReleaseLatch[b] = 0;
		}

		// Update active modifiers
		s_ActiveModifiers = 0;
		if (IsKeyDown(KeyCode::LeftShift) || IsKeyDown(KeyCode::RightShift))
			s_ActiveModifiers |= KeyModifier::Shift;
		if (IsKeyDown(KeyCode::LeftControl) || IsKeyDown(KeyCode::RightControl))
			s_ActiveModifiers |= KeyModifier::Control;
		if (IsKeyDown(KeyCode::LeftAlt) || IsKeyDown(KeyCode::RightAlt))
			s_ActiveModifiers |= KeyModifier::Alt;
		if (IsKeyDown(KeyCode::LeftSuper) || IsKeyDown(KeyCode::RightSuper))
			s_ActiveModifiers |= KeyModifier::Super;
	}

	bool Input::IsKeyPressed(KeyCode key)
	{
		EB_CORE_ASSERT(IN_KEY_RANGE(key), "Undefined key checked!");
		return s_KeyPressedSnapshot[static_cast<KeyCodeType>(key)] > 0;
	}

	bool Input::IsKeyReleased(KeyCode key)
	{
		EB_CORE_ASSERT(IN_KEY_RANGE(key), "Undefined key checked!");
		return s_KeyReleasedSnapshot[static_cast<KeyCodeType>(key)] > 0;
	}

	bool Input::IsKeyDown(KeyCode key)
	{
		EB_CORE_ASSERT(IN_KEY_RANGE(key), "Undefined key checked!");
		return s_KeyStates[static_cast<KeyCodeType>(key)] > 0;
	}

	bool Input::IsKeyRepeating(KeyCode key)
	{
		EB_CORE_ASSERT(IN_KEY_RANGE(key), "Undefined key checked!");
		// State > 1 means the key has been pressed AND repeated at least once
		return s_KeyStates[static_cast<KeyCodeType>(key)] > 1;
	}

	// MouseButton's three values are the first three MouseControls, so these are adapters.
	bool Input::IsMouseButtonDown(MouseButton button)
	{
		return IsMouseControlDown(static_cast<MouseControl>(button));
	}

	bool Input::IsMouseButtonPressed(MouseButton button)
	{
		return IsMouseControlPressed(static_cast<MouseControl>(button));
	}

	bool Input::IsMouseButtonReleased(MouseButton button)
	{
		return IsMouseControlReleased(static_cast<MouseControl>(button));
	}

	bool Input::IsMouseControlDown(MouseControl control)
	{
		EB_CORE_ASSERT(IN_MOUSE_CONTROL_RANGE(control), "Undefined mouse control checked!");
		return s_MouseControlStates[static_cast<size_t>(control)] > 0;
	}

	bool Input::IsMouseControlPressed(MouseControl control)
	{
		EB_CORE_ASSERT(IN_MOUSE_CONTROL_RANGE(control), "Undefined mouse control checked!");
		return s_MouseControlPressedSnapshot[static_cast<size_t>(control)] > 0;
	}

	bool Input::IsMouseControlReleased(MouseControl control)
	{
		EB_CORE_ASSERT(IN_MOUSE_CONTROL_RANGE(control), "Undefined mouse control checked!");
		return s_MouseControlReleasedSnapshot[static_cast<size_t>(control)] > 0;
	}

	Ember::KeyModifierType Input::GetActiveModifiers()
	{
		return s_ActiveModifiers;
	}

	bool Input::IsModifierActive(KeyModifier modifier)
	{
		return (s_ActiveModifiers & modifier);
	}

	void Input::SetKeyState(KeyCode key, bool pressed)
	{
		EB_CORE_ASSERT(IN_KEY_RANGE(key), "Undefined key pressed!");

		const size_t index = static_cast<KeyCodeType>(key);
		s_KeyStates[index] = pressed ? 1 : 0;

		// Latch the transition so a tap delivered entirely inside one glfwPollEvents batch is not
		// cancelled out before BeginFrame ever samples it.
		if (pressed)
			s_KeyPressLatch[index]++;
		else
			s_KeyReleaseLatch[index]++;
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

	void Input::SetMouseControlState(MouseControl control, bool pressed)
	{
		EB_CORE_ASSERT(IN_MOUSE_CONTROL_RANGE(control), "Undefined mouse control toggled!");

		const size_t index = static_cast<size_t>(control);
		s_MouseControlStates[index] = pressed;

		if (pressed)
			s_MouseControlPressLatch[index]++;
		else
			s_MouseControlReleaseLatch[index]++;
	}

	void Input::PulseMouseControl(MouseControl control)
	{
		EB_CORE_ASSERT(IN_MOUSE_CONTROL_RANGE(control), "Undefined mouse control pulsed!");

		// Both latches, and no level state: the next BeginFrame reports one pressed and one released
		// edge, and Down is never true. You cannot hold a wheel notch.
		const size_t index = static_cast<size_t>(control);
		s_MouseControlPressLatch[index]++;
		s_MouseControlReleaseLatch[index]++;
	}

	void Input::ClearAllStates()
	{
		s_KeyStates.fill(0);
		s_MouseControlStates.fill(0);
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

		// Derive the wheel controls here so every scroll source feeds them through one choke point.
		if (offset.y > 0.0f)
			PulseMouseControl(MouseControl::WheelUp);
		else if (offset.y < 0.0f)
			PulseMouseControl(MouseControl::WheelDown);
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

		if (Math::Abs(ret.x) < Input::MouseDeadzone)
		{
			ret.x = 0.0f;
		}

		if (Math::Abs(ret.y) < Input::MouseDeadzone)
		{
			ret.y = 0.0f;
		}

		return ret;
	}

	void Input::ResetMouseDelta()
	{
		s_PreviousMousePosition = s_MousePosition;
	}

	bool Input::IsAnyGamepadActive()
	{
		return std::any_of(s_GamepadStates.begin(), s_GamepadStates.end(),
			[](const GamepadState& state) { return state.Connected; });
	}
	
	bool Input::IsGamepadActive(size_t index)
	{
		return s_GamepadStates[index].Connected;
	}

	bool Input::IsGamepadButtonPressed(size_t index, GamepadButton button)
	{
		return s_GamepadStates[index].Down & (1 << static_cast<GamepadButtonType>(button)) 
			&& !(s_GamepadStates[index].PreviousDown & (1 << static_cast<GamepadButtonType>(button)));
	}

	bool Input::IsGamepadButtonReleased(size_t index, GamepadButton button)
	{
		return !(s_GamepadStates[index].Down & (1 << static_cast<GamepadButtonType>(button)));
	}

	bool Input::IsGamepadButtonDown(size_t index, GamepadButton button)
	{
		return s_GamepadStates[index].Down & (1 << static_cast<GamepadButtonType>(button));
	}

	float Input::GetGamepadAxis(size_t index, GamepadAxis axis)
	{
		return s_GamepadStates[index].Axis[static_cast<GamepadButtonType>(axis)];
	}

	void Input::SetGamepadButtonPressed(size_t index, GamepadButton button)
	{
		s_GamepadStates[index].Down |= (1 << static_cast<GamepadButtonType>(button));
	}

	void Input::SetGamepadButtonReleased(size_t index, GamepadButton button)
	{
		s_GamepadStates[index].Down &= ~(1 << static_cast<GamepadButtonType>(button));
	}

	void Input::SetGamepadAxis(size_t index, GamepadAxis axis, float strength)
	{
		s_GamepadStates[index].Axis[static_cast<GamepadButtonType>(axis)] = strength;
	}

	GamepadState& Input::GetGamepadState(size_t index)
	{
		return s_GamepadStates[index];
	}

	GamepadButtonMask Input::PressedMask(const GamepadState& s)
	{
		return s.Down & ~s.PreviousDown;
	}

	GamepadButtonMask Input::ReleasedMask(const GamepadState& s)
	{
		return ~s.Down & s.PreviousDown;
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