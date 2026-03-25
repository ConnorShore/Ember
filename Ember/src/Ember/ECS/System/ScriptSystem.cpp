#include "ebpch.h"
#include "ScriptSystem.h"

#include "Ember/Core/Application.h"
#include "Ember/Input/Input.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Scene/Entity.h"
#include "Ember/Scene/Behavior.h"
#include "Ember/Scene/Scene.h"
#include "Ember/Asset/Script.h"

namespace Ember {

	void ScriptSystem::OnAttach()
	{
		m_LuaState.open_libraries(sol::lib::base, sol::lib::math, sol::lib::os, sol::lib::string);
		BindAPI();
		EB_CORE_INFO("ScriptSystem attached!");
	}

	void ScriptSystem::OnDetach()
	{
	}

	void ScriptSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		auto view = scene->GetRegistry().Query<ScriptComponent>();

		for (auto entityID : view)
		{
			Entity entity{ entityID, scene };
			auto& script = entity.GetComponent<ScriptComponent>();

			if (script.FilePath.empty())
				continue;

			if (!script.Initialized)
			{
				auto scriptAsset = Application::Instance().GetAssetManager().Load<Script>(script.FilePath, m_LuaState);
				if (scriptAsset)
				{
					sol::table scriptClass = scriptAsset->GetClassTable();

					script.Instance = m_LuaState.create_table();
					script.Instance[sol::metatable_key] = m_LuaState.create_table_with("__index", scriptClass);

					sol::protected_function onCreate = scriptClass["OnCreate"];
					if (onCreate.valid())
					{
						onCreate(script.Instance, entity);
					}

					script.Initialized = true;
				}

				// Initialize no matter what so errors don't loop
				script.Initialized = true;
			}

			if (script.Initialized && script.Instance.valid())
			{
				sol::protected_function onUpdate = script.Instance["OnUpdate"];
				if (onUpdate.valid())
				{
					sol::protected_function_result updateResult = onUpdate(script.Instance, delta.Seconds());

					if (!updateResult.valid())
					{
						sol::error err = updateResult;
						EB_CORE_ERROR("Lua OnUpdate Error: {0}", err.what());
					}
				}
			}
		}
	}

	void ScriptSystem::BindAPI()
	{
		m_LuaState.new_usertype<Entity>("Entity",
			"GetTransform", [](Entity& e) -> TransformComponent& { return e.GetComponent<TransformComponent>(); },
			"GetName", & Entity::GetName,
			"GetUUID", & Entity::GetUUID
		);

		BindInput();
		BindMath();
		BindComponents();
	}

	void ScriptSystem::BindInput()
	{
		m_LuaState.new_enum("KeyCode",
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
		m_LuaState.new_enum("KeyAction",
			"Release", KeyAction::Release,
			"Press", KeyAction::Press,
			"Repeat", KeyAction::Repeat
		);

		m_LuaState.new_enum("KeyModifier",
			"None", KeyModifier::None,
			"Shift", KeyModifier::Shift,
			"Control", KeyModifier::Control,
			"Alt", KeyModifier::Alt,
			"Super", KeyModifier::Super
		);

		m_LuaState.set_function("IsKeyPressed", &Input::IsKeyPressed);
		m_LuaState.set_function("IsKeyHeld", &Input::IsKeyHeld);
	}

	void ScriptSystem::BindMath()
	{
		m_LuaState.new_usertype<Vector3f>("Vector3f",
			"x", &Vector3f::x,
			"y", &Vector3f::y,
			"z", &Vector3f::z
		);
	}

	void ScriptSystem::BindComponents()
	{
		m_LuaState.new_usertype<TransformComponent>("TransformComponent",
			"Position", &TransformComponent::Position,
			"Rotation", &TransformComponent::Rotation,
			"Scale", &TransformComponent::Scale
		);
	}

}