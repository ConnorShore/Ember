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
		static bool IsKeyPressed(KeyCode key);
		static bool IsKeyHeld(KeyCode key);
		static bool IsMouseButtonPressed(MouseButton button);
		static bool IsModifierActive(KeyModifier modifier);

		static void SetKeyState(KeyCode key, bool pressed);
		static void IncrementKeyRepeat(KeyCode key);
		static void SetMouseButtonState(MouseButton button, bool pressed);
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
		static constexpr size_t MouseButtonArraySize = static_cast<size_t>(MouseButton::Last);

		static std::array<int, KeyArraySize> s_KeyStates;
		static std::array<int, MouseButtonArraySize> s_MouseButtonStates;

		static KeyModifierType s_ActiveModifiers;

		static Vector2f s_MousePosition, s_ScrollOffset, s_PreviousMousePosition;

		static Vector2f s_ViewportMin, s_ViewportSize;
		static bool s_ViewportInputActive;
	};
}