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

		auto inputTable = state.create_named_table("Input");
		inputTable.set_function("IsKeyPressed", &Input::IsKeyDown);
		inputTable.set_function("IsKeyHeld", &Input::IsKeyRepeating);
		inputTable.set_function("IsMouseButtonPressed", &Input::IsMouseButtonDown);
		
		inputTable.set_function("GetMousePosition", &Input::GetMousePosition);
		inputTable.set_function("GetMouseScrollOffset", &Input::GetMouseScrollOffset);
		inputTable.set_function("GetMouseDelta", &Input::GetMouseDelta);
		
		inputTable.set_function("SetCursorMode", &Input::SetCursorMode);
		inputTable.set_function("GetCursorMode", &Input::GetCursorMode);

		// Viewport-local, bottom-left origin - the space UI rects are laid out in. Unlike
		// GetMousePosition this is correct inside the editor's docked viewport during Play.
		inputTable.set_function("GetViewportMousePosition", &Input::GetViewportMousePosition);

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
