#include "ebpch.h"

#include "Ember/Core/Application.h"
#include "Ember/ECS/System/UIInputSystem.h"
#include "ScriptBindInput.h"

#include "Ember/Input/Input.h"

namespace Ember {

	void BindInput(sol::state& state)
	{
		// Key bindings - enumerators come from KeyCodes.inl so the Lua enum cannot drift from the C++ one.
		state.new_enum("KeyCode",
			"Unknown", KeyCode::Unknown,
#define EB_KEY(name, value) #name, KeyCode::name,
#include "Ember/Input/KeyCodes.inl"
#undef EB_KEY
			"Last", KeyCode::Last
		);

		state.new_enum("KeyAction",
			"Release", KeyAction::Release,
			"Press", KeyAction::Press,
			"Repeat", KeyAction::Repeat
		);

		state.new_enum("KeyModifier",
			"None", KeyModifier::None,
			"Shift", KeyModifier::Shift,
			"Control", KeyModifier::Control,
			"Alt", KeyModifier::Alt,
			"Super", KeyModifier::Super
		);
		// Mouse bindings
		state.new_enum("MouseButton",
			"Left", MouseButton::Left,
			"Right", MouseButton::Right,
			"Middle", MouseButton::Middle
		);

		// Every bindable mouse control, from MouseControls.inl. The macro emits a leading comma
		// because the list has no trailing sentinel to close the argument pack with.
		state.new_enum("MouseControl"
#define EB_MOUSE_CONTROL(name, value) , #name, MouseControl::name
#include "Ember/Input/MouseControls.inl"
#undef EB_MOUSE_CONTROL
		);

		state.new_enum("CursorMode",
			"Normal", CursorMode::Normal,
			"Hidden", CursorMode::Hidden,
			"Locked", CursorMode::Locked
		);

		state.new_enum("InputDevice",
			"None", InputDevice::None,
			"Keyboard", InputDevice::Keyboard,
			"Mouse", InputDevice::Mouse,
			"Gamepad", InputDevice::Gamepad
		);

		// Gamepad controls come from GamepadButton.inl / GamepadAxis.inl, same as the key list, and
		// use the leading-comma form because neither table has a sentinel to close the pack with.
		state.new_enum("GamepadButton"
#define EB_GAMEPAD_BUTTON(name, value) , #name, GamepadButton::name
#include "Ember/Input/GamepadButton.inl"
#undef EB_GAMEPAD_BUTTON
			, "Last", GamepadButton::Last
		);

		state.new_enum("GamepadAxis"
#define EB_GAMEPAD_AXIS(name, value) , #name, GamepadAxis::name
#include "Ember/Input/GamepadAxis.inl"
#undef EB_GAMEPAD_AXIS
			, "Last", GamepadAxis::Last
		);

		state.new_enum("GamepadStick",
			"Left", GamepadStick::Left,
			"Right", GamepadStick::Right
		);

		state.new_enum("GamepadTrigger",
			"Left", GamepadTrigger::Left,
			"Right", GamepadTrigger::Right
		);

		state.new_usertype<StickSettings>("StickSettings",
			"Deadzone", &StickSettings::Deadzone,
			"Saturation", &StickSettings::Saturation,
			"Exponent", &StickSettings::Exponent,
			"Actuation", &StickSettings::Actuation,
			"InvertX", &StickSettings::InvertX,
			"InvertY", &StickSettings::InvertY
		);

		state.new_usertype<TriggerSettings>("TriggerSettings",
			"Deadzone", &TriggerSettings::Deadzone,
			"Saturation", &TriggerSettings::Saturation,
			"Exponent", &TriggerSettings::Exponent,
			"Actuation", &TriggerSettings::Actuation
		);

		state.new_usertype<MouseSettings>("MouseSettings",
			"InvertX", &MouseSettings::InvertX,
			"InvertY", &MouseSettings::InvertY
		);

		auto inputTable = state.create_named_table("Input");
		inputTable.set_function("IsKeyDown", &Input::IsKeyDown);
		inputTable.set_function("IsKeyReleased", &Input::IsKeyReleased);
		inputTable.set_function("IsKeyRepeating", &Input::IsKeyRepeating);

		inputTable.set_function("IsMouseControlDown", &Input::IsMouseControlDown);
		inputTable.set_function("IsMouseControlPressed", &Input::IsMouseControlPressed);
		inputTable.set_function("IsMouseControlReleased", &Input::IsMouseControlReleased);

		inputTable.set_function("IsMouseButtonDown", &Input::IsMouseButtonDown);
		inputTable.set_function("IsMouseButtonReleased", &Input::IsMouseButtonReleased);

		// Legacy spellings: these two have always meant "held" to game scripts, so they stay wired
		// to the level state rather than the one-frame edge the C++ names of the same shape return.
		inputTable.set_function("IsKeyPressed", &Input::IsKeyDown);
		inputTable.set_function("IsMouseButtonPressed", &Input::IsMouseButtonDown);

		// Modifiers are a bitmask, so a multi-bit argument means "any of these", not "all of them".
		// Compare GetActiveModifiers() yourself when a chord has to match exactly.
		inputTable.set_function("IsModifierDown", &Input::IsModifierActive);
		inputTable.set_function("IsModifierActive", &Input::IsModifierActive);
		inputTable.set_function("GetActiveModifiers", &Input::GetActiveModifiers);

		inputTable.set_function("GetMousePosition", &Input::GetMousePosition);
		inputTable.set_function("GetMouseScrollOffset", &Input::GetMouseScrollOffset);
		inputTable.set_function("GetMouseDelta", &Input::GetMouseDelta);
		inputTable.set_function("GetRawMouseDelta", &Input::GetRawMouseDelta);

		// Pointers, not references: sol2 copies a returned reference, which would make a script's
		// write land on a temporary and silently do nothing.
		inputTable.set_function("GetStickSettings", [](GamepadStick stick) { return &Input::GetStickSettings(stick); });
		inputTable.set_function("GetTriggerSettings", [](GamepadTrigger trigger) { return &Input::GetTriggerSettings(trigger); });
		inputTable.set_function("GetMouseSettings", []() { return &Input::GetMouseSettings(); });

		auto& inputActionManager = Application::Instance().GetInputActionManager();
		inputTable.set_function("IsActionDown", [&inputActionManager](std::string_view actionName) { return inputActionManager.IsActionDown(actionName); });
		inputTable.set_function("IsActionPressed", [&inputActionManager](std::string_view actionName) { return inputActionManager.IsActionPressed(actionName); });
		inputTable.set_function("IsActionReleased", [&inputActionManager](std::string_view actionName) { return inputActionManager.IsActionReleased(actionName); });
		inputTable.set_function("GetActionStrength", [&inputActionManager](std::string_view actionName) { return inputActionManager.GetActionStrength(actionName); });

		// For a script that has acted on a press and does not want the rest of the frame acting on it
		// too - every action sharing that physical button goes quiet until it is released.
		inputTable.set_function("ConsumeAction", [&inputActionManager](std::string_view actionName) { inputActionManager.ConsumeAction(actionName); });

		inputTable.set_function("GetAxis", [&inputActionManager](std::string_view negative, std::string_view positive) { return inputActionManager.GetAxis(negative, positive); });
		inputTable.set_function("GetAxis2D", [&inputActionManager](std::string_view left, std::string_view right, std::string_view down, std::string_view up) { return inputActionManager.GetAxis2D(left, right, down, up); });
		
		inputTable.set_function("SetCursorMode", &Input::SetCursorMode);
		inputTable.set_function("GetCursorMode", &Input::GetCursorMode);

		// Viewport-local, bottom-left origin - the space UI rects are laid out in. Unlike
		// GetMousePosition this is correct inside the editor's docked viewport during Play.
		inputTable.set_function("GetViewportMousePosition", &Input::GetViewportMousePosition);

		// Pad index is 0-based, so player one is 0.
		inputTable.set_function("IsAnyGamepadActive", &Input::IsAnyGamepadActive);
		inputTable.set_function("IsGamepadActive", &Input::IsGamepadActive);
		inputTable.set_function("IsGamepadButtonDown", &Input::IsGamepadButtonDown);
		inputTable.set_function("IsGamepadButtonPressed", &Input::IsGamepadButtonPressed);
		inputTable.set_function("IsGamepadButtonReleased", &Input::IsGamepadButtonReleased);
		inputTable.set_function("GetGamepadAxis", &Input::GetGamepadAxis);

		// What the player touched last, so prompts can switch between key and button glyphs.
		inputTable.set_function("GetLastUsedInputDevice", &Input::GetLastUsedInputDevice);
		inputTable.set_function("SetLastUsedInputDevice", &Input::SetLastUsedInputDevice);

		auto uiTable = state.create_named_table("UI");

		// Lets gameplay ignore a click that landed on the HUD.
		uiTable.set_function("IsPointerOverUI", []()
			{
				auto uiInputSystem = Application::Instance().GetSystemManager().GetSystem<UIInputSystem>();
				return uiInputSystem && uiInputSystem->IsPointerOverUI();
			});

		uiTable.set_function("ClearFocus", []()
			{
				if (auto uiInputSystem = Application::Instance().GetSystemManager().GetSystem<UIInputSystem>())
					uiInputSystem->SetFocusedEntity((EntityID)Constants::Entities::InvalidEntityID);
			});
	}

}
