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

	InputSettings Input::s_Settings = {};

	KeyModifierType Input::s_ActiveModifiers = 0;
	Vector2f Input::s_MousePosition = { 0.0f, 0.0f };
	Vector2f Input::s_ScrollOffset = { 0.0f, 0.0f };
	Vector2f Input::s_PreviousMousePosition = { 0.0f, 0.0f };

	Vector2f Input::s_ViewportMin = { 0.0f, 0.0f };
	Vector2f Input::s_ViewportSize = { 0.0f, 0.0f };

	bool Input::s_ViewportInputActive = true;
	bool Input::s_GameplayInputSuppressed = false;

	InputDevice Input::s_LastUsedDevice = InputDevice::None;

	void Input::BeginFrame()
	{
		// Ahead of the key/mouse snapshots so every reader this frame sees conditioned axes.
		ProcessGamepads();

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
		// Leaves the frame baseline ResetMouseDelta set alone: GLFW delivers a callback per mouse
		// report, so rebaselining on each one would leave only the last hop of the frame.
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

	Vector2f Input::GetRawMouseDelta()
	{
		// Zero rather than frozen: a look script reads the delta unconditionally, so anything else
		// here would keep steering the camera while the editor owns the pointer.
		if (s_GameplayInputSuppressed)
			return Vector2f(0.0f);

		return s_MousePosition - s_PreviousMousePosition;
	}

	Vector2f Input::GetMouseDelta()
	{
		Vector2f delta = GetRawMouseDelta();

		if (s_Settings.Mouse.InvertX)
		{
			delta.x = -delta.x;
		}

		if (s_Settings.Mouse.InvertY)
		{
			delta.y = -delta.y;
		}

		return delta;
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
		// Writes both sides: callers are injecting a stick position, and the conditioned read has to
		// answer immediately rather than only after the next BeginFrame.
		GamepadState& state = s_GamepadStates[index];
		state.RawAxis[static_cast<GamepadButtonType>(axis)] = strength;
		state.Axis[static_cast<GamepadButtonType>(axis)] = strength;
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

	InputSettings& Input::GetSettings()
	{
		return s_Settings;
	}

	StickSettings& Input::GetStickSettings(GamepadStick stick)
	{
		return s_Settings.Sticks[static_cast<size_t>(stick)];
	}

	TriggerSettings& Input::GetTriggerSettings(GamepadTrigger trigger)
	{
		return s_Settings.Triggers[static_cast<size_t>(trigger)];
	}

	MouseSettings& Input::GetMouseSettings()
	{
		return s_Settings.Mouse;
	}

	bool Input::IsTriggerAxis(GamepadAxis axis)
	{
		return axis == GamepadAxis::LeftTrigger || axis == GamepadAxis::RightTrigger;
	}

	GamepadStick Input::StickForAxis(GamepadAxis axis)
	{
		return (axis == GamepadAxis::RightX || axis == GamepadAxis::RightY)
			? GamepadStick::Right
			: GamepadStick::Left;
	}

	GamepadTrigger Input::TriggerForAxis(GamepadAxis axis)
	{
		return axis == GamepadAxis::RightTrigger ? GamepadTrigger::Right : GamepadTrigger::Left;
	}

	float Input::ShapeMagnitude(float magnitude, float deadzone, float saturation, float exponent)
	{
		if (magnitude < deadzone)
		{
			return 0.0f;
		}

		// Saturation can be dialled below the deadzone by a careless edit; Epsilon keeps the divide
		// finite rather than handing back an infinity that would latch the control on.
		const float span = Math::Max(saturation - deadzone, Math::Epsilon);
		const float normalized = Math::Clamp((magnitude - deadzone) / span, 0.0f, 1.0f);

		return Math::Pow(normalized, exponent);
	}

	float Input::ActuationThreshold(GamepadStick stick)
	{
		const StickSettings& settings = GetStickSettings(stick);
		return ShapeMagnitude(settings.Actuation, settings.Deadzone, settings.Saturation, settings.Exponent);
	}

	float Input::ActuationThreshold(GamepadTrigger trigger)
	{
		const TriggerSettings& settings = GetTriggerSettings(trigger);
		return ShapeMagnitude(settings.Actuation, settings.Deadzone, settings.Saturation, settings.Exponent);
	}

	bool Input::ProcessStick(GamepadState& state, GamepadStick stick, GamepadAxis xAxis, GamepadAxis yAxis)
	{
		const StickSettings& settings = GetStickSettings(stick);
		const size_t x = static_cast<size_t>(xAxis);
		const size_t y = static_cast<size_t>(yAxis);

		// Always from RawAxis, never from Axis - conditioning has to be idempotent, since BeginFrame
		// can legitimately run more than once between polls.
		const Vector2f raw = { state.RawAxis[x], state.RawAxis[y] };
		const float magnitude = Math::Length(raw);
		const float shaped = ShapeMagnitude(magnitude, settings.Deadzone, settings.Saturation, settings.Exponent);

		if (shaped == 0.0f)
		{
			state.Axis[x] = 0.0f;
			state.Axis[y] = 0.0f;
			return false;
		}

		// Shaped on the magnitude and re-projected, rather than per axis: a per-axis curve bends a
		// diagonal off its true angle and makes a circular sweep of the stick come out lumpy.
		const float scale = shaped / magnitude;
		state.Axis[x] = raw.x * scale * (settings.InvertX ? -1.0f : 1.0f);
		state.Axis[y] = raw.y * scale * (settings.InvertY ? -1.0f : 1.0f);

		return true;
	}

	bool Input::ProcessTrigger(GamepadState& state, GamepadTrigger trigger, GamepadAxis axis)
	{
		const TriggerSettings& settings = GetTriggerSettings(trigger);
		const size_t index = static_cast<size_t>(axis);

		// A trigger rests at -1 and bottoms out at +1, so it is remapped before anything else.
		const float travel = (state.RawAxis[index] + 1.0f) * 0.5f;
		state.Axis[index] = ShapeMagnitude(travel, settings.Deadzone, settings.Saturation, settings.Exponent);

		return state.Axis[index] != 0.0f;
	}

	void Input::ProcessGamepads()
	{
		bool anyUsed = false;

		for (size_t i = 0; i < MaxGamepads; ++i)
		{
			GamepadState& state = s_GamepadStates[i];
			if (!state.Connected)
			{
				continue;
			}

			anyUsed |= ProcessStick(state, GamepadStick::Left, GamepadAxis::LeftX, GamepadAxis::LeftY);
			anyUsed |= ProcessStick(state, GamepadStick::Right, GamepadAxis::RightX, GamepadAxis::RightY);
			anyUsed |= ProcessTrigger(state, GamepadTrigger::Left, GamepadAxis::LeftTrigger);
			anyUsed |= ProcessTrigger(state, GamepadTrigger::Right, GamepadAxis::RightTrigger);
			anyUsed |= state.Down != 0;
		}

		// Device tracking lives here rather than in the platform poll because a stick resting inside
		// its deadzone is noise, not the player reaching for the pad.
		if (anyUsed)
		{
			SetLastUsedInputDevice(InputDevice::Gamepad);
		}
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

	void Input::SetGameplayInputSuppressed(bool suppressed)
	{
		s_GameplayInputSuppressed = suppressed;
	}

	bool Input::IsGameplayInputSuppressed()
	{
		return s_GameplayInputSuppressed;
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