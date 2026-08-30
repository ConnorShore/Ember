#pragma once

#include <array>

#include "InputCode.h"
#include "GamepadState.h"
#include "Ember/Math/Math.h"
#include "Ember/Core/CursorMode.h"

namespace Ember {

	class Input
	{
	public:
		// Defaults only - these stay writable so they can be driven from user settings later.
		// Mouse delta floor in pixels; gamepad axes arrive normalized, so they need their own floors.
		inline static float MouseDeadzone = 1.5f;
		inline static float GamepadStickDeadzone = 0.15f;
		inline static float GamepadTriggerDeadzone = 0.05f;

		inline static constexpr size_t MaxGamepads = 4;

	public:
		static void BeginFrame();

		static bool IsKeyPressed(KeyCode key);
		static bool IsKeyReleased(KeyCode key);
		static bool IsKeyDown(KeyCode key);
		static bool IsKeyRepeating(KeyCode key);

		static bool IsMouseButtonDown(MouseButton button);
		static bool IsMouseButtonPressed(MouseButton button);
		static bool IsMouseButtonReleased(MouseButton button);

		static bool IsMouseControlDown(MouseControl control);
		static bool IsMouseControlPressed(MouseControl control);
		static bool IsMouseControlReleased(MouseControl control);

		static KeyModifierType GetActiveModifiers();
		static bool IsModifierActive(KeyModifier modifier);

		static void SetKeyState(KeyCode key, bool pressed);
		static void IncrementKeyRepeat(KeyCode key);
		static void SetMouseControlState(MouseControl control, bool pressed);

		// A wheel notch has no held state and GLFW sends no release, so it is recorded as a
		// one-frame pulse: pressed and released on the same BeginFrame.
		static void PulseMouseControl(MouseControl control);
		static void SetKeyModifierState(KeyModifier modifier, bool active);

		// Clear every cached key / mouse button / modifier state. Called when
		// the window loses focus so that keys released while unfocused (e.g.
		// while a native file dialog is up) can't appear stuck on return.
		static void ClearAllStates();

		static void SetMousePosition(const Vector2f& position);
		inline static void SetMousePosition(float x, float y) { SetMousePosition({ x, y }); }

		static void UpdateMousePosition(const Vector2f& position);
		static void SetMouseScrollOffset(const Vector2f& offset);

		static void SetCursorMode(CursorMode mode);
		static CursorMode GetCursorMode();

		static int GetKeyRepeatCount(KeyCode key);
		static const Vector2f& GetMousePosition();
		static const Vector2f& GetMouseScrollOffset();
		static Vector2f GetMouseDelta();

		static void ResetMouseDelta();

		static void SetGamepadControlPressed(size_t index, GamepadButton button);
		static void SetGamepadControlReleased(size_t index, GamepadButton button);
		static void SetGamepadAxis(size_t index, GamepadAxis axis, float strength);
		static GamepadState& GetGamepadState(size_t index);
		static GamepadButtonMask PressedMask(const GamepadState& s);
		static GamepadButtonMask ReleasedMask(const GamepadState& s);

		// Logs each connected pad's held buttons and off-centre axes. A debug aid, not a hot path.
		static void PrintActiveGamepadControls();

		// The game viewport within the window, in window coords (top-left origin, +Y down).
		// In Ember-Runtime this is the whole window; in Ember-Forge it is the docked viewport panel.
		static void SetViewportRect(const Vector2f& min, const Vector2f& size, bool inputActive = true);
		static const Vector2f& GetViewportMin();
		static const Vector2f& GetViewportSize();

		// False when another surface (a floating editor panel) owns the pointer.
		static bool IsViewportInputActive();
		static bool IsMouseInViewport();

		// Mouse in UI space: viewport-local, bottom-left origin, +Y up, matching UILayoutSystem rects.
		static Vector2f GetViewportMousePosition();

	private:
		// Key State Tracking
		static constexpr size_t KeyArraySize = static_cast<size_t>(KeyCode::Last);

		static std::array<int, KeyArraySize> s_KeyStates;
		static std::array<uint8_t, KeyArraySize> s_KeyDownSnapshot;
		static std::array<uint8_t, KeyArraySize> s_KeyPressedSnapshot;
		static std::array<uint8_t, KeyArraySize> s_KeyReleasedSnapshot;
		static std::array<uint8_t, KeyArraySize> s_KeyPressLatch;        // presses since BeginFrame
		static std::array<uint8_t, KeyArraySize> s_KeyReleaseLatch;
		static KeyModifierType s_ActiveModifiers;

		// Mosue State Tracking
		static constexpr size_t MouseControlArraySize = static_cast<size_t>(MouseControl::Last);

		static std::array<int, MouseControlArraySize> s_MouseControlStates;
		static std::array<uint8_t, MouseControlArraySize> s_MouseControlDownSnapshot;
		static std::array<uint8_t, MouseControlArraySize> s_MouseControlPressedSnapshot;
		static std::array<uint8_t, MouseControlArraySize> s_MouseControlReleasedSnapshot;
		static std::array<uint8_t, MouseControlArraySize> s_MouseControlPressLatch;        // presses since BeginFrame
		static std::array<uint8_t, MouseControlArraySize> s_MouseControlReleaseLatch;
		static Vector2f s_MousePosition, s_ScrollOffset, s_PreviousMousePosition;

		// Gamepad State Tracking
		static std::array<GamepadState, MaxGamepads> s_GamepadStates;

		// Viewport State Tracking
		static Vector2f s_ViewportMin, s_ViewportSize;
		static bool s_ViewportInputActive;
	};
}