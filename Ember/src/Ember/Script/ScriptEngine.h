#pragma once

#include "ScriptProperty.h"
#include "Ember/Script/Script.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Core/Application.h"

#include <sol/sol.hpp>
#include <vector>
#include <array>

namespace Ember {

	class Scene;
	struct ScriptComponent;

	enum class UICallbackKind
	{
		Click = 0,
		ValueChanged
	};

	class ScriptEngine
	{
	public:
		static void Init();
		static void Shutdown();

		static void BindAPI(Scene* scene);

		static void OnRuntimeStart(Scene* scene);
		static void OnRuntimeStop(Scene* scene);

		static sol::state& GetState();

		// `visitedBaseChain` tracks the UUIDs of scripts already walked while resolving a Base
		// chain, so a cycle (A.Base = B, B.Base = A) is detected and reported instead of recursing
		// forever. Callers outside ScriptEngine should never pass it explicitly.
		static std::vector<ScriptProperty> GetScriptProperties(const SharedPtr<Script>& scriptAsset,
			std::vector<UUID> visitedBaseChain = {});

		static sol::object ScriptPropertyValueToLua(sol::state& luaState, const ScriptPropertyValue& value);

		// Walks `scriptClass`'s `Base` field (if any), chaining each class table's metatable
		// __index to its resolved parent so inherited methods/fields resolve through the
		// instance -> class -> base -> ... lookup chain. `scriptName` is only used for cycle
		// detection and error messages. No-op if the script has no Base field.
		static void ResolveScriptInheritance(sol::table scriptClass, const std::string& scriptName);

		// Schedules a Lua callback to run after delaySeconds have elapsed.
		static void SetTimeout(sol::protected_function callback, float delaySeconds);
		static void UpdateTimeouts(TimeStep delta);
		static void ClearTimeouts();

		// UI callbacks registered from Lua at runtime, for dynamically built screens (store rows,
		// inventory lists) where an editor-authored hook is not enough.
		//
		// Deliberately owned here rather than stored on the component: ScriptEngine destroys and
		// recreates the whole sol::state on runtime start/stop, and the runtime scene outlives that
		// teardown - a sol handle sitting in a component would destruct against a dead lua_State.
		// Keyed by UUID because EntityIDs are recycled, so a stale entry here is inert.
		static void RegisterUICallback(UUID entity, UICallbackKind kind, sol::protected_function callback);
		static void ClearUICallbacks(UUID entity, UICallbackKind kind);
		static void ClearAllUICallbacks();
		static void InvokeUICallbacks(Scene* scene, EntityID entity, UICallbackKind kind);
		static void InvokeUICallbacks(Scene* scene, EntityID entity, UICallbackKind kind, bool argument);

		// Backs the Lua-facing EventManager.Subscribe/Broadcast global (see ScriptBindEvents.cpp).
		// Owned here rather than in a script table for the same reason as the UI callbacks above:
		// listeners are sol handles into a sol::state ScriptEngine destroys and recreates on runtime
		// start/stop, so they must be torn down alongside it rather than living on some entity.
		static void SubscribeEvent(const std::string& eventName, sol::protected_function callback);
		static void BroadcastEvent(const std::string& eventName, sol::variadic_args args);
		static void ClearEventListeners();

		template<typename T>
		static void SetScriptPropertyOverride(ScriptComponent& component, const std::string& propertyName, T value)
		{
			// Extract ScriptPropertyType from T
			ScriptPropertyType type;
			if constexpr (std::same_as<T, bool>)
				type = ScriptPropertyType::Bool;
			else if constexpr (std::same_as<T, int>)
				type = ScriptPropertyType::Int;
			else if constexpr (std::same_as<T, float>)
				type = ScriptPropertyType::Float;
			else if constexpr (std::same_as<T, std::string>)
				type = ScriptPropertyType::String;
			else if constexpr (std::same_as<T, Vector3f>)
				type = ScriptPropertyType::Vector3f;
			else if constexpr (std::same_as<T, uint64_t> || std::same_as<T, UUID>)
				type = ScriptPropertyType::AssetRef; // Or whatever enum you use for references
			else
			{
				EB_CORE_ASSERT(false, "Unsupported type passed in for prop override!");
				type = ScriptPropertyType::Unknown;
			}
			
			// Set property override
			component.UserPropertyOverrides[propertyName] = { propertyName, value, type };
		}

		// Enum overrides preserve the option list so the editor combo can re-render after a value change.
		static void SetScriptEnumPropertyOverride(ScriptComponent& component, const std::string& propertyName,
			int value, std::vector<std::pair<std::string, int>> enumOptions)
		{
			component.UserPropertyOverrides[propertyName] =
				{ propertyName, value, ScriptPropertyType::Enum, std::move(enumOptions) };
		}

		static void SetScriptReferencePropertyOverride(ScriptComponent& component, const std::string& propertyName,
			UUID value, ScriptPropertyType type, ScriptReferenceKind referenceKind)
		{
			component.UserPropertyOverrides[propertyName] = { propertyName, value, type, referenceKind };
		}

		// Reference arrays are overridden whole - a script's declared array is only ever empty.
		static void SetScriptReferenceArrayPropertyOverride(ScriptComponent& component, const std::string& propertyName,
			std::vector<UUID> values, ScriptReferenceKind referenceKind)
		{
			component.UserPropertyOverrides[propertyName] =
				{ propertyName, std::move(values), ScriptPropertyType::ReferenceArray, referenceKind };
		}

		inline static std::array<std::string, 10> DefaultEmberFunctions = {
			"OnCreate",
			"OnUpdate",
			"OnOverlapTriggerEnter",
			"OnOverlapTriggerStay",
			"OnOverlapTriggerExit",
			"OnAnimationEvent",
			"OnClick",
			"OnValueChanged",
			"OnHoverEnter",
			"OnHoverExit"
		};

		// Reserved field name a script table sets to another script asset's name to inherit from
		// it, e.g. `MyScript.Base = "MyParentScript"`. Excluded from exposed-property enumeration.
		inline static const std::string BaseFieldName = "Base";

		// Engine-set (not author-set) reserved field holding this script's full ancestry - its own
		// name followed by each resolved Base in order. Entity:GetScriptInstance(name) checks this
		// so a name matching any ancestor resolves, not just the concrete script's own name.
		// Excluded from exposed-property enumeration.
		inline static const std::string BaseChainFieldName = "__baseChain";
	};

}