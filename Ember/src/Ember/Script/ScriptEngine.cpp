#include "ebpch.h"
#include "ScriptEngine.h"

#include "Ember/Core/Application.h"
#include "Ember/Input/Input.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Scene/Entity.h"
#include "Ember/Scene/Scene.h"

namespace Ember {

	static sol::state* s_LuaState = nullptr;

	void ScriptEngine::Init()
	{
		EB_CORE_INFO("Initializing Script Engine...");
	}

	void ScriptEngine::Shutdown()
	{
		if (s_LuaState)
			OnRuntimeStart();

		EB_CORE_INFO("Shutdown Script Engine...");
	}

	void ScriptEngine::OnRuntimeStart()
	{
		s_LuaState = new sol::state();
		s_LuaState->open_libraries(sol::lib::base, sol::lib::math, sol::lib::os, sol::lib::string);
		BindAPI();

		EB_CORE_INFO("Started Script Engine runtime!");
	}

	void ScriptEngine::OnRuntimeStop()
	{
		delete s_LuaState;
		s_LuaState = nullptr;
		EB_CORE_INFO("Stopped Script Engine runtime!");
	}

	sol::state& ScriptEngine::GetState()
	{
		EB_CORE_ASSERT(s_LuaState, "Attempted to access Lua State while it is dead! Are you in Play Mode?");
		return *s_LuaState;
	}

	void ScriptEngine::BindAPI()
	{
		s_LuaState->new_usertype<Entity>("Entity",
			"GetTransform", [](Entity& e) -> TransformComponent& { return e.GetComponent<TransformComponent>(); },
			"GetName", &Entity::GetName,
			"GetUUID", &Entity::GetUUID
		);

		BindInput();
		BindMath();
		BindComponents();
	}

	void ScriptEngine::BindInput()
	{
		s_LuaState->new_enum("KeyCode",
			"Unknown", KeyCode::Unknown,
			"Space", KeyCode::Space,
			"Apostrophe", KeyCode::Apostrophe,
			"Comma", KeyCode::Comma,
			"Minus", KeyCode::Minus,
			"Period", KeyCode::Period,
			"Slash", KeyCode::Slash,
			"D0", KeyCode::D0,
			"D1", KeyCode::D1,
			"D2", KeyCode::D2,
			"D3", KeyCode::D3,
			"D4", KeyCode::D4,
			"D5", KeyCode::D5,
			"D6", KeyCode::D6,
			"D7", KeyCode::D7,
			"D8", KeyCode::D8,
			"D9", KeyCode::D9,
			"Semicolon", KeyCode::Semicolon,
			"Equal", KeyCode::Equal,
			"A", KeyCode::A,
			"B", KeyCode::B,
			"C", KeyCode::C,
			"D", KeyCode::D,
			"E", KeyCode::E,
			"F", KeyCode::F,
			"G", KeyCode::G,
			"H", KeyCode::H,
			"I", KeyCode::I,
			"J", KeyCode::J,
			"K", KeyCode::K,
			"L", KeyCode::L,
			"M", KeyCode::M,
			"N", KeyCode::N,
			"O", KeyCode::O,
			"P", KeyCode::P,
			"Q", KeyCode::Q,
			"R", KeyCode::R,
			"S", KeyCode::S,
			"T", KeyCode::T,
			"U", KeyCode::U,
			"V", KeyCode::V,
			"W", KeyCode::W,
			"X", KeyCode::X,
			"Y", KeyCode::Y,
			"Z", KeyCode::Z,
			"LeftBracket", KeyCode::LeftBracket,
			"Backslash", KeyCode::Backslash,
			"RightBracket", KeyCode::RightBracket,
			"GraveAccent", KeyCode::GraveAccent,

			// Function keys
			"Escape", KeyCode::Escape,
			"Enter", KeyCode::Enter,
			"Tab", KeyCode::Tab,
			"Backspace", KeyCode::Backspace,
			"Insert", KeyCode::Insert,
			"Delete", KeyCode::Delete,
			"Right", KeyCode::Right,
			"Left", KeyCode::Left,
			"Down", KeyCode::Down,
			"Up", KeyCode::Up,
			"PageUp", KeyCode::PageUp,
			"PageDown", KeyCode::PageDown,
			"Home", KeyCode::Home,
			"End", KeyCode::End,
			"CapsLock", KeyCode::CapsLock,
			"ScrollLock", KeyCode::ScrollLock,
			"NumLock", KeyCode::NumLock,
			"PrintScreen", KeyCode::PrintScreen,
			"Pause", KeyCode::Pause,
			"F1", KeyCode::F1,
			"F2", KeyCode::F2,
			"F3", KeyCode::F3,
			"F4", KeyCode::F4,
			"F5", KeyCode::F5,
			"F6", KeyCode::F6,
			"F7", KeyCode::F7,
			"F8", KeyCode::F8,
			"F9", KeyCode::F9,
			"F10", KeyCode::F10,
			"F11", KeyCode::F11,
			"F12", KeyCode::F12,

			// NumPad
			"NumPad0", KeyCode::NumPad0,
			"NumPad1", KeyCode::NumPad1,
			"NumPad2", KeyCode::NumPad2,
			"NumPad3", KeyCode::NumPad3,
			"NumPad4", KeyCode::NumPad4,
			"NumPad5", KeyCode::NumPad5,
			"NumPad6", KeyCode::NumPad6,
			"NumPad7", KeyCode::NumPad7,
			"NumPad8", KeyCode::NumPad8,
			"NumPad9", KeyCode::NumPad9,
			"NumPadDecimal", KeyCode::NumPadDecimal,
			"NumPadDivide", KeyCode::NumPadDivide,
			"NumPadMultiply", KeyCode::NumPadMultiply,
			"NumPadSubtract", KeyCode::NumPadSubtract,
			"NumPadAdd", KeyCode::NumPadAdd,
			"NumPadEnter", KeyCode::NumPadEnter,
			"NumPadEqual", KeyCode::NumPadEqual,

			// Modifier keys
			"LeftShift", KeyCode::LeftShift,
			"LeftControl", KeyCode::LeftControl,
			"LeftAlt", KeyCode::LeftAlt,
			"LeftSuper", KeyCode::LeftSuper,
			"RightShift", KeyCode::RightShift,
			"RightControl", KeyCode::RightControl,
			"RightAlt", KeyCode::RightAlt,
			"RightSuper", KeyCode::RightSuper,
			"Menu", KeyCode::Menu,
			"Last", KeyCode::Last
		);

		// ------------------------------------------------------------------------
		// KEY ACTIONS
		// ------------------------------------------------------------------------
		s_LuaState->new_enum("KeyAction",
			"Release", KeyAction::Release,
			"Press", KeyAction::Press,
			"Repeat", KeyAction::Repeat
		);

		s_LuaState->new_enum("KeyModifier",
			"None", KeyModifier::None,
			"Shift", KeyModifier::Shift,
			"Control", KeyModifier::Control,
			"Alt", KeyModifier::Alt,
			"Super", KeyModifier::Super
		);

		s_LuaState->set_function("IsKeyPressed", &Input::IsKeyPressed);
		s_LuaState->set_function("IsKeyHeld", &Input::IsKeyHeld);
	}

	void ScriptEngine::BindMath()
	{
		s_LuaState->new_usertype<Vector3f>("Vector3f",
			"x", &Vector3f::x,
			"y", &Vector3f::y,
			"z", &Vector3f::z
		);
	}

	void ScriptEngine::BindComponents()
	{
		s_LuaState->new_usertype<TransformComponent>("TransformComponent",
			"Position", &TransformComponent::Position,
			"Rotation", &TransformComponent::Rotation,
			"Scale", &TransformComponent::Scale
		);
	}

}