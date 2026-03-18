#pragma once

#include <array>

#include "InputCode.h"
#include "Ember/Math/Math.h"

namespace Ember {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);
		static bool IsKeyHeld(KeyCode key);
		static bool IsMouseButtonPressed(MouseButton button);
		static bool IsModifierActive(KeyModifier modifier);

		static void SetKeyState(KeyCode key, bool pressed);
		static void IncrementKeyRepeat(KeyCode key);
		static void SetMouseButtonState(MouseButton button, bool pressed);
		static void SetKeyModifierState(KeyModifier modifier, bool active);
		static void SetMousePosition(const Vector2f& position);
		static void SetMouseScrollOffset(const Vector2f& offset);

		static int GetKeyRepeatCount(KeyCode key);
		static const Vector2f& GetMousePosition();
		static const Vector2f& GetMouseScrollOffset();

	private:
		static constexpr size_t KeyArraySize = static_cast<size_t>(KeyCode::Last);
		static constexpr size_t MouseButtonArraySize = static_cast<size_t>(MouseButton::Last);

		static std::array<int, KeyArraySize> s_KeyStates;
		static std::array<int, MouseButtonArraySize> s_MouseButtonStates;

		static KeyModifierType s_ActiveModifiers;

		// TODO: Vector2 for mouse position, scroll offset, etc.
		static Vector2f s_MousePosition, s_ScrollOffset;
	};
}