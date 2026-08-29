#pragma once

#include <array>

#include "InputCode.h"
#include "Ember/Math/Math.h"
#include "Ember/Core/CursorMode.h"

namespace Ember {

	class Input
	{
	public:
		inline static const float Deadzone = 1.5f;	// 1.5 pixels

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
		static constexpr size_t KeyArraySize = static_cast<size_t>(KeyCode::Last);
		static constexpr size_t MouseControlArraySize = static_cast<size_t>(MouseControl::Last);

		static std::array<int, KeyArraySize> s_KeyStates;
		static std::array<uint8_t, KeyArraySize> s_KeyDownSnapshot;
		static std::array<uint8_t, KeyArraySize> s_KeyPressedSnapshot;
		static std::array<uint8_t, KeyArraySize> s_KeyReleasedSnapshot;
		static std::array<uint8_t, KeyArraySize> s_KeyPressLatch;        // presses since BeginFrame
		static std::array<uint8_t, KeyArraySize> s_KeyReleaseLatch;

		static std::array<int, MouseControlArraySize> s_MouseControlStates;
		static std::array<uint8_t, MouseControlArraySize> s_MouseControlDownSnapshot;
		static std::array<uint8_t, MouseControlArraySize> s_MouseControlPressedSnapshot;
		static std::array<uint8_t, MouseControlArraySize> s_MouseControlReleasedSnapshot;
		static std::array<uint8_t, MouseControlArraySize> s_MouseControlPressLatch;        // presses since BeginFrame
		static std::array<uint8_t, MouseControlArraySize> s_MouseControlReleaseLatch;

		static KeyModifierType s_ActiveModifiers;
		static Vector2f s_MousePosition, s_ScrollOffset, s_PreviousMousePosition;

		static Vector2f s_ViewportMin, s_ViewportSize;
		static bool s_ViewportInputActive;
	};
}