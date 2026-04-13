#include "ebpch.h"
#include "ScriptEngine.h"

#include "Ember/Core/Application.h"
#include "Ember/Input/Input.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Scene/Entity.h"
#include "Ember/Scene/Scene.h"

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Utility helpers
	//////////////////////////////////////////////////////////////////////////

	static sol::object GetAssetFromTypeString(const std::string& assetTypeStr, const std::string& assetName)
	{
		auto& assetManager = Application::Instance().GetAssetManager();
		AssetType type = GetAssetTypeFromString(assetTypeStr);
		switch (type)
		{
			case AssetType::Texture:
				return sol::make_object(ScriptEngine::GetState(), assetManager.GetAsset<Texture>(assetName).Ptr());
			case AssetType::Mesh:
				return sol::make_object(ScriptEngine::GetState(), assetManager.GetAsset<Mesh>(assetName).Ptr());
			case AssetType::Model:
				return sol::make_object(ScriptEngine::GetState(), assetManager.GetAsset<Model>(assetName).Ptr());
			case AssetType::Skeleton:
				return sol::make_object(ScriptEngine::GetState(), assetManager.GetAsset<Skeleton>(assetName).Ptr());
			case AssetType::Animation:
				return sol::make_object(ScriptEngine::GetState(), assetManager.GetAsset<Animation>(assetName).Ptr());
			case AssetType::Shader:
				return sol::make_object(ScriptEngine::GetState(), assetManager.GetAsset<Shader>(assetName).Ptr());
			case AssetType::Material:
				return sol::make_object(ScriptEngine::GetState(), assetManager.GetAsset<Material>(assetName).Ptr());
			case AssetType::Script:
				EB_CORE_ASSERT(false, "Cannot get script assets from Lua!");
				return sol::lua_nil;
			default:
				EB_CORE_ASSERT(false, "Unknown asset type: {}", assetTypeStr);
				return sol::lua_nil;
		}
	}

	static sol::object GetComponentFromString(const std::string& componentTypeStr, Entity& entity)
	{
		if (componentTypeStr == "IDComponent")
			return sol::make_object(ScriptEngine::GetState(), &entity.GetComponent<IDComponent>());
		if (componentTypeStr == "TagComponent")
			return sol::make_object(ScriptEngine::GetState(), &entity.GetComponent<TagComponent>());
		if (componentTypeStr == "RelationshipComponent")
			return sol::make_object(ScriptEngine::GetState(), &entity.GetComponent<RelationshipComponent>());
		if (componentTypeStr == "TransformComponent")
			return sol::make_object(ScriptEngine::GetState(), &entity.GetComponent<TransformComponent>());
		if (componentTypeStr == "RigidBodyComponent")
			return sol::make_object(ScriptEngine::GetState(), &entity.GetComponent<RigidBodyComponent>());
		if (componentTypeStr == "SpriteComponent")
			return sol::make_object(ScriptEngine::GetState(), &entity.GetComponent<SpriteComponent>());
		if (componentTypeStr == "StaticMeshComponent")
			return sol::make_object(ScriptEngine::GetState(), &entity.GetComponent<StaticMeshComponent>());
		if (componentTypeStr == "SkinnedMeshComponent")
			return sol::make_object(ScriptEngine::GetState(), &entity.GetComponent<SkinnedMeshComponent>());
		if (componentTypeStr == "MaterialComponent")
			return sol::make_object(ScriptEngine::GetState(), &entity.GetComponent<MaterialComponent>());
		if (componentTypeStr == "CameraComponent")
			return sol::make_object(ScriptEngine::GetState(), &entity.GetComponent<CameraComponent>());
		if (componentTypeStr == "DirectionalLightComponent")
			return sol::make_object(ScriptEngine::GetState(), &entity.GetComponent<DirectionalLightComponent>());
		if (componentTypeStr == "SpotLightComponent")
			return sol::make_object(ScriptEngine::GetState(), &entity.GetComponent<SpotLightComponent>());
		if (componentTypeStr == "PointLightComponent")
			return sol::make_object(ScriptEngine::GetState(), &entity.GetComponent<PointLightComponent>());
		if (componentTypeStr == "OutlineComponent")
			return sol::make_object(ScriptEngine::GetState(), &entity.GetComponent<OutlineComponent>());
		if (componentTypeStr == "BillboardComponent")
			return sol::make_object(ScriptEngine::GetState(), &entity.GetComponent<BillboardComponent>());
		if (componentTypeStr == "AnimatorComponent")
			return sol::make_object(ScriptEngine::GetState(), &entity.GetComponent<AnimatorComponent>());

		if (componentTypeStr == "ScriptComponent")
		{
			EB_CORE_ASSERT(false, "Cannot get script components from Lua!");
			return sol::lua_nil;
		}

		EB_CORE_ASSERT(false, "Unknown component type: {}", componentTypeStr);
		return sol::lua_nil;
	}

	static sol::object ContainsComponentFromString(const std::string& componentTypeStr, Entity& entity)
	{
		if (componentTypeStr == "IDComponent")
			return sol::make_object(ScriptEngine::GetState(), entity.ContainsComponent<IDComponent>());
		if (componentTypeStr == "TagComponent")
			return sol::make_object(ScriptEngine::GetState(), entity.ContainsComponent<TagComponent>());
		if (componentTypeStr == "RelationshipComponent")
			return sol::make_object(ScriptEngine::GetState(), entity.ContainsComponent<RelationshipComponent>());
		if (componentTypeStr == "TransformComponent")
			return sol::make_object(ScriptEngine::GetState(), entity.ContainsComponent<TransformComponent>());
		if (componentTypeStr == "RigidBodyComponent")
			return sol::make_object(ScriptEngine::GetState(), entity.ContainsComponent<RigidBodyComponent>());
		if (componentTypeStr == "SpriteComponent")
			return sol::make_object(ScriptEngine::GetState(), entity.ContainsComponent<SpriteComponent>());
		if (componentTypeStr == "StaticMeshComponent")
			return sol::make_object(ScriptEngine::GetState(), entity.ContainsComponent<StaticMeshComponent>());
		if (componentTypeStr == "SkinnedMeshComponent")
			return sol::make_object(ScriptEngine::GetState(), entity.ContainsComponent<SkinnedMeshComponent>());
		if (componentTypeStr == "MaterialComponent")
			return sol::make_object(ScriptEngine::GetState(), entity.ContainsComponent<MaterialComponent>());
		if (componentTypeStr == "CameraComponent")
			return sol::make_object(ScriptEngine::GetState(), entity.ContainsComponent<CameraComponent>());
		if (componentTypeStr == "DirectionalLightComponent")
			return sol::make_object(ScriptEngine::GetState(), entity.ContainsComponent<DirectionalLightComponent>());
		if (componentTypeStr == "SpotLightComponent")
			return sol::make_object(ScriptEngine::GetState(), entity.ContainsComponent<SpotLightComponent>());
		if (componentTypeStr == "PointLightComponent")
			return sol::make_object(ScriptEngine::GetState(), entity.ContainsComponent<PointLightComponent>());
		if (componentTypeStr == "OutlineComponent")
			return sol::make_object(ScriptEngine::GetState(), entity.ContainsComponent<OutlineComponent>());
		if (componentTypeStr == "BillboardComponent")
			return sol::make_object(ScriptEngine::GetState(), entity.ContainsComponent<BillboardComponent>());
		if (componentTypeStr == "AnimatorComponent")
			return sol::make_object(ScriptEngine::GetState(), entity.ContainsComponent<AnimatorComponent>());

		if (componentTypeStr == "ScriptComponent")
		{
			EB_CORE_ASSERT(false, "Cannot check for script components from Lua!");
			return sol::lua_nil;
		}

		EB_CORE_ASSERT(false, "Unknown component type: {}", componentTypeStr);
		return sol::lua_nil;
	}

	//////////////////////////////////////////////////////////////////////////
	// Script Engine
	//////////////////////////////////////////////////////////////////////////

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

	// Creates a fresh Lua VM for each play session so scripts start with clean state
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
		BindCore();
		BindEntity();
		BindInput();
		BindMath();
		BindComponents();
		BindAssets();
	}

	void ScriptEngine::BindCore()
	{
		s_LuaState->new_usertype<UUID>("UUID",
			sol::constructors<UUID(), UUID(uint64_t)>(),
            sol::meta_function::equal_to, static_cast<bool(UUID::*)(const UUID&) const>(&UUID::operator==),
			sol::meta_function::to_string, [](const UUID& a) { return std::to_string((uint64_t)a); }
		);

		// Logging
		s_LuaState->set_function("Log", [](const std::string& message) {
			EB_CORE_INFO("{}", message);
			});

		s_LuaState->set_function("LogWarn", [](const std::string& message) {
			EB_CORE_WARN("{}", message);
			});

		s_LuaState->set_function("LogError", [](const std::string& message) {
			EB_CORE_ERROR("{}", message);
			});
	}

	void ScriptEngine::BindEntity()
	{
		s_LuaState->new_usertype<Entity>("Entity",
			"GetName", &Entity::GetName,
			"GetUUID", &Entity::GetUUID,
			"GetComponent", [](Entity& e, const std::string& componentTypeStr) { return GetComponentFromString(componentTypeStr, e); },
			"ContainsComponent", [](Entity& e, const std::string& componentTypeStr) { return ContainsComponentFromString(componentTypeStr, e); }
		);
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
			sol::constructors<Vector3f(), Vector3f(float, float, float)>(),
			"x", &Vector3f::x,
			"y", &Vector3f::y,
			"z", &Vector3f::z
		);
	}

	void ScriptEngine::BindComponents()
	{
		// TODO: Setup bindings for all components
		s_LuaState->new_usertype<TransformComponent>("TransformComponent",
			"Position", &TransformComponent::Position,
			"Rotation", &TransformComponent::Rotation,
			"Scale", &TransformComponent::Scale,
			"GetForward", &TransformComponent::GetForward
		);

		s_LuaState->new_usertype<RigidBodyComponent>("RigidBodyComponent",
			"Mass", &RigidBodyComponent::Mass,
			"GravityEnabled", &RigidBodyComponent::GravityEnabled,
			"ApplyForce", &RigidBodyComponent::ApplyForce,
			"ApplyImpulse", &RigidBodyComponent::ApplyImpulse
		);

		s_LuaState->new_usertype<AnimatorComponent>("AnimatorComponent",
			"CurrentAnimationHandle", &AnimatorComponent::CurrentAnimationHandle,
			"CurrentTime", sol::property(
				[](AnimatorComponent& c) { return (float)c.CurrentTime; },           // Getter (returns float to Lua)
				[](AnimatorComponent& c, float time) { c.CurrentTime = time; }       // Setter (takes float from Lua)
			),
			"PlaybackSpeed", &AnimatorComponent::PlaybackSpeed,
			"IsPlaying", &AnimatorComponent::IsPlaying,
			"Loop", &AnimatorComponent::Loop,
			"Crossfade", [](AnimatorComponent& c, UUID targetAnim, float duration) {
				if (c.CurrentAnimationHandle == targetAnim || targetAnim == Constants::InvalidUUID)
					return;
				if (c.CurrentAnimationHandle == Constants::InvalidUUID)
				{
					// No current animation, just switch immediately
					c.CurrentAnimationHandle = targetAnim;
					c.CurrentTime = 0.0f;
					c.IsPlaying = true;
					return;
				}

				c.PreviousAnimationHandle = c.CurrentAnimationHandle;
				c.PreviousTime = c.CurrentTime;
				c.CurrentAnimationHandle = targetAnim;
				c.CurrentTime = 0.0f;
				c.BlendDuration = duration;
				c.CurrentBlendTime = 0.0f;
				c.IsPlaying = true;
			}
		);
	}

	void ScriptEngine::BindAssets()
	{
		// Define assets
		s_LuaState->new_usertype<Animation>("Animation",
			"GetUUID", [](Animation& anim) { return anim.GetUUID(); },
			"GetName", [](Animation& anim) { return anim.GetName(); },
			"GetDuration", [](Animation& anim) { return anim.GetDuration(); }
		);

		// Asset Manager table
		sol::table assetManager = s_LuaState->create_named_table("AssetManager");

		// Ability to get assets
		assetManager.set_function("GetAsset", [](const std::string& typeName, const std::string& assetName, sol::this_state s) -> sol::object {
			return GetAssetFromTypeString(typeName, assetName);
		});
	}

}