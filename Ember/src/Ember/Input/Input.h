#pragma once

#include <array>

#include "InputDevice.h"
#include "InputCode.h"
#include "GamepadState.h"
#include "InputSettings.h"
#include "Ember/Math/Math.h"
#include "Ember/Core/CursorMode.h"

namespace Ember {

	class Input
	{
	public:
		inline static constexpr size_t MaxGamepads = 4;

	public:
		static void BeginFrame();

		// How each device is conditioned before anything reads it - defaults from the .ebproj, and
		// writable at runtime by a settings menu.
		static InputSettings& GetSettings();
		static StickSettings& GetStickSettings(GamepadStick stick);
		static TriggerSettings& GetTriggerSettings(GamepadTrigger trigger);
		static MouseSettings& GetMouseSettings();

		// The actuation point put through the same conditioning, so a response curve cannot silently
		// move every digital threshold on that control.
		static float ActuationThreshold(GamepadStick stick);
		static float ActuationThreshold(GamepadTrigger trigger);

		// Deadzone, saturation and curve on a 0..1 deflection. Public so the editor can plot the same
		// curve the sticks, the triggers and the actuation thresholds all run through.
		static float ShapeMagnitude(float magnitude, float deadzone, float saturation, float exponent);

		// Which settings block an axis belongs to.
		static GamepadStick StickForAxis(GamepadAxis axis);
		static GamepadTrigger TriggerForAxis(GamepadAxis axis);
		static bool IsTriggerAxis(GamepadAxis axis);

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

		// The whole frame's movement, with the player's inversion preference applied.
		static Vector2f GetMouseDelta();

		// True pixels, ignoring inversion - for UI dragging and editor tooling, which want the
		// pointer's actual travel rather than a look preference.
		static Vector2f GetRawMouseDelta();

		static void ResetMouseDelta();

		static bool IsAnyGamepadActive();
		static bool IsGamepadActive(size_t index);

		static bool IsGamepadButtonPressed(size_t index, GamepadButton button);
		static bool IsGamepadButtonReleased(size_t index, GamepadButton button);
		static bool IsGamepadButtonDown(size_t index, GamepadButton button);
		static float GetGamepadAxis(size_t index, GamepadAxis axis);

		static void SetGamepadButtonPressed(size_t index, GamepadButton button);
		static void SetGamepadButtonReleased(size_t index, GamepadButton button);
		static void SetGamepadAxis(size_t index, GamepadAxis axis, float strength);

		static GamepadState& GetGamepadState(size_t index);
		static GamepadButtonMask PressedMask(const GamepadState& s);
		static GamepadButtonMask ReleasedMask(const GamepadState& s);

		// The game viewport within the window, in window coords (top-left origin, +Y down).
		// In Ember-Runtime this is the whole window; in Ember-Forge it is the docked viewport panel.
		static void SetViewportRect(const Vector2f& min, const Vector2f& size, bool inputActive = true);
		static const Vector2f& GetViewportMin();
		static const Vector2f& GetViewportSize();

		// False when another surface (a floating editor panel) owns the pointer.
		static bool IsViewportInputActive();
		static bool IsMouseInViewport();

		// Set while the editor has taken input back from a running game. Distinct from the cursor
		// mode, which a game is free to change for its own menus while it still wants input.
		static void SetGameplayInputSuppressed(bool suppressed);
		static bool IsGameplayInputSuppressed();

		// Mouse in UI space: viewport-local, bottom-left origin, +Y up, matching UILayoutSystem rects.
		static Vector2f GetViewportMousePosition();

		inline static InputDevice GetLastUsedInputDevice() { return s_LastUsedDevice; }
		inline static void SetLastUsedInputDevice(InputDevice device) { s_LastUsedDevice = device; }

	private:
		// Raw to conditioned for every connected pad, at the top of BeginFrame - after the platform
		// layer has polled and before actions are evaluated.
		static void ProcessGamepads();
		static bool ProcessStick(GamepadState& state, GamepadStick stick, GamepadAxis xAxis, GamepadAxis yAxis);
		static bool ProcessTrigger(GamepadState& state, GamepadTrigger trigger, GamepadAxis axis);

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

		static InputSettings s_Settings;

		// Viewport State Tracking
		static Vector2f s_ViewportMin, s_ViewportSize;
		static bool s_ViewportInputActive;
		static bool s_GameplayInputSuppressed;

		static InputDevice s_LastUsedDevice;
	};
}