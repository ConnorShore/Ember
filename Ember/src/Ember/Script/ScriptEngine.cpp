#include "ebpch.h"
#include "ScriptEngine.h"

#include "Bindings/ScriptBindCore.h"
#include "Bindings/ScriptBindEntity.h"
#include "Bindings/ScriptBindInput.h"
#include "Bindings/ScriptBindMath.h"
#include "Ember/Math/Math.h"
#include "Bindings/ScriptBindPhysics.h"
#include "Bindings/ScriptBindComponents.h"
#include "Bindings/ScriptBindAssets.h"
#include "Bindings/ScriptBindScene.h"
#include "Bindings/ScriptBindAudio.h"
#include "Bindings/ScriptBindSaveGame.h"
#include "Bindings/ScriptBindDebugDraw.h"

#include "Ember/Core/Core.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Scene/Scene.h"

#include <algorithm>
#include <fstream>
#include <optional>
#include <unordered_map>

namespace Ember {

    //////////////////////////////////////////////////////////////////////////
    // Script Engine
    //////////////////////////////////////////////////////////////////////////

    static sol::state* s_LuaState = nullptr;

	struct TimeoutRequest
	{
		float RemainingSeconds = 0.0f;
		sol::protected_function Callback;
	};

	static std::vector<TimeoutRequest> s_Timeouts;

	// Tracks whether the scene-independent bindings have been registered against the *current*
	// state. Re-running new_usertype for a type that is already registered on a live state leaves
	// previously handed-out values unreliable: Entity userdata intermittently comes back with no
	// usable __index, so `entity:AnyMethod()` fails from Lua. A play session binds a fresh state
	// exactly once and never noticed, but anything that binds one state repeatedly degrades.
	static bool s_StatelessBindingsRegistered = false;

	// Generational GC instead of Lua 5.4's default incremental collector: the runtime allocates many
	// short-lived Lua objects per frame, which is exactly the pattern generational GC smooths out.
	static sol::state* CreateConfiguredLuaState()
	{
		sol::state* state = new sol::state();
		state->open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
		lua_gc(state->lua_state(), LUA_GCGEN, 0, 0);

		// Cleared here rather than at the call sites because this is the only place a state is
		// created. Keying the guard off the state pointer would not be safe - a freshly allocated
		// state can reuse the address of the one just deleted.
		s_StatelessBindingsRegistered = false;
		return state;
	}

	// Registers everything that does not capture a Scene. Safe to call as often as you like; the
	// work happens once per Lua state.
	static void BindStatelessAPI()
	{
		if (s_StatelessBindingsRegistered)
			return;

		BindCore(*s_LuaState);
		BindEntity(*s_LuaState);
		BindInput(*s_LuaState);
		BindMath(*s_LuaState);
		BindAssets(*s_LuaState);
		BindSaveGame(*s_LuaState);
		BindDebugDraw(*s_LuaState);

		// Every component binder except the AI one is scene-independent.
		BindCoreComponents(*s_LuaState);
		BindPhysicsComponents(*s_LuaState);
		BindRenderingComponents(*s_LuaState);
		BindLightingAndCameraComponents(*s_LuaState);
		BindAudioComponents(*s_LuaState);
		BindMiscComponents(*s_LuaState);

		s_StatelessBindingsRegistered = true;
	}

    void ScriptEngine::Init()
    {
		// Create the state immediately so the Editor can parse scripts!
		s_LuaState = CreateConfiguredLuaState();
		ClearTimeouts();

		// Everything scene-independent, so a script referencing UUID/ref helpers, Vector3f defaults
		// or GameData at module scope still parses in the editor. Going through the same guarded
		// path as BindAPI is what keeps these from being registered a second time later.
		BindStatelessAPI();

		EB_CORE_INFO("ScriptEngine Initialized (Editor State)");
    }

    void ScriptEngine::Shutdown()
	{
		ClearTimeouts();
		delete s_LuaState;
		s_LuaState = nullptr;

        EB_CORE_INFO("Shutdown Script Engine...");
    }

	void ScriptEngine::BindAPI(Scene* scene)
	{
		BindStatelessAPI();

		// These four capture `scene`, so they are rebound on every call to point at the new one.
		BindScene(*s_LuaState, scene);
		BindPhysics(*s_LuaState, scene);
		BindAIComponents(*s_LuaState, scene);
		BindAudio(*s_LuaState, scene);
	}

    // Creates a fresh Lua VM for each play session so scripts start with clean state
	void ScriptEngine::OnRuntimeStart(Scene* scene)
	{
		// wipe editor state and create fresh one for runtime
		ClearTimeouts();
		delete s_LuaState;
		s_LuaState = CreateConfiguredLuaState();

		// 3. Bind the Engine API to this fresh runtime state
		BindAPI(scene);
	}

	void ScriptEngine::OnRuntimeStop(Scene* scene)
	{
		// Null the sol::table in every ScriptComponent before destroying the Lua state. Otherwise the
		// scene outlives it and those destructors touch a dead lua_State, faulting inside gettable.
		if (scene)
		{
			auto& registry = scene->GetRegistry();
			auto view = registry.Query<ScriptComponent>();
			for (EntityID entity : view)
			{
				auto& sc = registry.GetComponent<ScriptComponent>(entity);
				sc.Instance = sol::table{};
				sc.Initialized = false;
			}
		}

		// Wipe runtime state and create fresh one for the editor
		ClearTimeouts();
		delete s_LuaState;
		s_LuaState = CreateConfiguredLuaState();

		// Same scene-independent set the editor starts with; the scene-capturing bindings are left
		// off until the next BindAPI, since there is no active runtime scene to point them at.
		BindStatelessAPI();
	}

    sol::state& ScriptEngine::GetState()
    {
        EB_CORE_ASSERT(s_LuaState, "Attempted to access Lua State while it is dead! Are you in Play Mode?");
        return *s_LuaState;
    }

	sol::object ScriptEngine::ScriptPropertyValueToLua(sol::state& luaState, const ScriptPropertyValue& value)
	{
		return std::visit([&](const auto& typedValue) -> sol::object {
			return sol::make_object(luaState, typedValue);
		}, value);
	}

	void ScriptEngine::ResolveScriptInheritance(sol::table scriptClass, const std::string& scriptName)
	{
		std::vector<std::string> visited = { scriptName };
		sol::table current = scriptClass;

		while (true)
		{
			sol::optional<std::string> baseScriptName = current[BaseFieldName];
			if (!baseScriptName)
				break;

			if (std::find(visited.begin(), visited.end(), baseScriptName.value()) != visited.end())
			{
				EB_CORE_ERROR("Script inheritance cycle detected: '{}' Base chain revisits '{}'. Ignoring Base from that point on.",
					scriptName, baseScriptName.value());
				break;
			}

			auto& assetManager = Application::Instance().GetAssetManager();
			if (!assetManager.ContainsAssetWithName(baseScriptName.value()))
			{
				EB_CORE_ERROR("Script '{}' declares Base = '{}' but no script asset with that name exists.",
					scriptName, baseScriptName.value());
				break;
			}

			auto baseAsset = assetManager.GetAsset<Script>(baseScriptName.value());
			if (!baseAsset)
			{
				EB_CORE_ERROR("Script '{}' declares Base = '{}' but that name belongs to a non-Script asset.",
					scriptName, baseScriptName.value());
				break;
			}

			// Scoped so the stack-based result is released before the next iteration loads another
			// chunk - see the note in GetScriptProperties about nested results mis-indexing the stack.
			sol::table baseClass;
			{
				sol::protected_function_result baseResult = GetState().script_file(baseAsset->GetFilePath());
				if (!baseResult.valid())
				{
					sol::error err = baseResult;
					EB_CORE_ERROR("Failed to load base script '{}' for inheritance: {}", baseScriptName.value(), err.what());
					break;
				}
				baseClass = baseResult.get<sol::table>();
			}

			current[sol::metatable_key] = GetState().create_table_with("__index", baseClass);

			visited.push_back(baseScriptName.value());
			current = baseClass;
		}

		// Record the full ancestry (this script's own name, then each resolved Base in order) on
		// the class table so Entity:GetScriptInstance(name) can recognize this script as "is-a" any
		// of its ancestors - not just its own concrete name. Stamped even when there's no Base at
		// all, so the lookup below always has a chain to check.
		sol::table baseChain = GetState().create_table();
		for (size_t i = 0; i < visited.size(); ++i)
			baseChain[i + 1] = visited[i];
		scriptClass[BaseChainFieldName] = baseChain;
	}

	//////////////////////////////////////////////////////////////////////////
	// GetScriptProperties helpers
	//////////////////////////////////////////////////////////////////////////

	// Handles the two supported table-shaped property kinds: an asset/entity reference wrapper
	// (tagged with __ember_property_type == "Reference") or a string-keyed table of integers, which
	// is treated as an enum (e.g. `Pickup.Kind = { Ammo = 1, Health = 2 }`).
	static std::optional<ScriptProperty> ParseTableScriptProperty(sol::state& luaState, const std::string& name, sol::table tableValue)
	{
		sol::optional<std::string> propertyType = tableValue["__ember_property_type"];
		if (propertyType && propertyType.value() == "Reference")
		{
			sol::optional<std::string> kindName = tableValue["Kind"];
			ScriptReferenceKind referenceKind = ScriptReferenceKindFromString(kindName.value_or("Asset"));

			uint64_t rawValue = (uint64_t)Constants::InvalidUUID;
			sol::optional<uint64_t> numericValue = tableValue["Value"];
			if (numericValue)
				rawValue = numericValue.value();

			ScriptProperty property;
			property.Name = name;
			property.Type = referenceKind == ScriptReferenceKind::Entity ? ScriptPropertyType::EntityRef : ScriptPropertyType::AssetRef;
			property.Value = UUID(rawValue);
			property.ReferenceKind = referenceKind;
			return property;
		}

		// Treat string-keyed tables of integers as enums, e.g. Pickup.Kind = { Ammo = 1, Health = 2 }
		std::vector<std::pair<std::string, int>> enumOptions;
		bool isEnum = !tableValue.empty();
		for (auto& [enumKey, enumValue] : tableValue)
		{
			if (enumKey.get_type() != sol::type::string || enumValue.get_type() != sol::type::number)
			{
				isEnum = false;
				break;
			}

			enumValue.push();
			bool isInteger = lua_isinteger(luaState, -1);
			lua_pop(luaState, 1);
			if (!isInteger)
			{
				isEnum = false;
				break;
			}

			enumOptions.emplace_back(enumKey.as<std::string>(), enumValue.as<int>());
		}

		if (!isEnum || enumOptions.empty())
		{
			EB_CORE_WARN("Unsupported script property type for '{}'", name);
			return std::nullopt;
		}

		// Keep declaration order stable so the editor combo matches the script
		std::sort(enumOptions.begin(), enumOptions.end(),
			[](const auto& a, const auto& b) { return a.second < b.second; });

		ScriptProperty property;
		property.Name = name;
		property.Type = ScriptPropertyType::Enum;
		property.Value = enumOptions.front().second; // default to first option
		property.EnumOptions = std::move(enumOptions);
		return property;
	}

	// Classifies a single (name, value) pair from a script's raw table into a ScriptProperty, or
	// returns nullopt if the value isn't a supported property type (e.g. an unsupported userdata,
	// or a table that's neither a reference wrapper nor an enum).
	static std::optional<ScriptProperty> ParseScriptPropertyValue(sol::state& luaState, const std::string& name, sol::object value)
	{
		switch (value.get_type())
		{
			case sol::type::number:
			{
				// Push value to top of stack so we can inspect it
				value.push();
				bool isInteger = lua_isinteger(luaState, -1);
				lua_pop(luaState, 1); // Pop it off

				ScriptProperty property;
				property.Name = name;
				if (isInteger)
				{
					property.Type = ScriptPropertyType::Int;
					property.Value = value.as<int>();
				}
				else
				{
					property.Type = ScriptPropertyType::Float;
					property.Value = value.as<float>();
				}
				return property;
			}
			case sol::type::string:
			{
				ScriptProperty property;
				property.Name = name;
				property.Type = ScriptPropertyType::String;
				property.Value = value.as<std::string>();
				return property;
			}
			case sol::type::boolean:
			{
				ScriptProperty property;
				property.Name = name;
				property.Type = ScriptPropertyType::Bool;
				property.Value = value.as<bool>();
				return property;
			}
			case sol::type::userdata:
			{
				// Detect Vector3f userdata values exposed as script properties
				// e.g. MyScript.MyVec = Vector3f.new(1.0, 2.0, 3.0)
				if (!value.is<Vector3f>())
				{
					EB_CORE_WARN("Unsupported userdata script property type for '{}'", name);
					return std::nullopt;
				}

				ScriptProperty property;
				property.Name = name;
				property.Type = ScriptPropertyType::Vector3f;
				property.Value = value.as<Vector3f>();
				return property;
			}
			case sol::type::table:
				return ParseTableScriptProperty(luaState, name, value.as<sol::table>());
			default:
				EB_CORE_WARN("Unsupported script property type for '{}'", name);
				return std::nullopt; // Skip unsupported types
		}
	}

	// Merges properties inherited via Base into `properties`, skipping any name this script already
	// declares itself (a child's own field always shadows the inherited default). No-op if the
	// script has no Base field.
	static void MergeInheritedProperties(const SharedPtr<Script>& scriptAsset, sol::table scriptClass,
		std::vector<ScriptProperty>& properties, const std::vector<UUID>& visitedBaseChain)
	{
		sol::optional<std::string> baseScriptName = scriptClass[ScriptEngine::BaseFieldName];
		if (!baseScriptName)
			return;

		auto& assetManager = Application::Instance().GetAssetManager();
		if (!assetManager.ContainsAssetWithName(baseScriptName.value()))
		{
			EB_CORE_ERROR("Script '{}' declares Base = '{}' but no script asset with that name exists.",
				scriptAsset->GetName(), baseScriptName.value());
			return;
		}

		auto baseAsset = assetManager.GetAsset<Script>(baseScriptName.value());
		if (!baseAsset)
		{
			EB_CORE_ERROR("Script '{}' declares Base = '{}' but that name belongs to a non-Script asset.",
				scriptAsset->GetName(), baseScriptName.value());
			return;
		}

		if (std::find(visitedBaseChain.begin(), visitedBaseChain.end(), baseAsset->GetUUID()) != visitedBaseChain.end())
		{
			EB_CORE_ERROR("Script inheritance cycle detected: '{}' Base chain revisits '{}'.",
				scriptAsset->GetName(), baseAsset->GetName());
			return;
		}

		std::vector<UUID> chain = visitedBaseChain;
		chain.push_back(scriptAsset->GetUUID());

		auto baseProperties = ScriptEngine::GetScriptProperties(baseAsset, chain);
		for (auto& baseProperty : baseProperties)
		{
			bool overridden = std::any_of(properties.begin(), properties.end(),
				[&baseProperty](const ScriptProperty& p) { return p.Name == baseProperty.Name; });
			if (!overridden)
				properties.push_back(baseProperty);
		}
	}

	// Sorts `properties` to match the order their names first appear as "Name =" in the script's
	// source file, so the editor lists them in the same order the author wrote them. Properties that
	// don't appear in the file (i.e. inherited via Base) keep their relative merge order and sort
	// after every property the file itself declares.
	static void SortPropertiesByDeclarationOrder(const std::string& filePath, std::vector<ScriptProperty>& properties)
	{
		std::ifstream scriptFile(filePath);
		if (!scriptFile.is_open())
			return;

		std::unordered_map<std::string, int> lineNumbers;
		std::string line;
		int lineNum = 0;
		while (std::getline(scriptFile, line))
		{
			++lineNum;
			for (auto& prop : properties)
			{
				if (lineNumbers.count(prop.Name))
					continue;
				// Match "PropName" followed by optional whitespace then "="
				// but not "==" so we don't match comparisons
				auto pos = line.find(prop.Name);
				if (pos == std::string::npos)
					continue;

				auto after = pos + prop.Name.size();
				while (after < line.size() && line[after] == ' ') ++after;
				if (after < line.size() && line[after] == '=' && (after + 1 >= line.size() || line[after + 1] != '='))
					lineNumbers[prop.Name] = lineNum;
			}
		}

		std::stable_sort(properties.begin(), properties.end(),
			[&lineNumbers](const ScriptProperty& a, const ScriptProperty& b)
			{
				int la = lineNumbers.count(a.Name) ? lineNumbers[a.Name] : INT_MAX;
				int lb = lineNumbers.count(b.Name) ? lineNumbers[b.Name] : INT_MAX;
				return la < lb;
			});
	}

	std::vector<ScriptProperty> ScriptEngine::GetScriptProperties(const SharedPtr<Script>& scriptAsset, std::vector<UUID> visitedBaseChain)
	{
		// Evaluate the script to get the base table. Scoped so this stack-based result is released
		// before MergeInheritedProperties recurses (each Base level re-enters script_file); sol2
		// tears a result down with lua_remove, which shifts every slot above it, so leaving one
		// open across that recursion mis-indexes the stack.
		std::string filePath = scriptAsset->GetFilePath();
		sol::table scriptClass;
		{
			sol::protected_function_result result = GetState().script_file(filePath);
			if (!result.valid())
			{
				sol::error err = result;
				EB_CORE_ERROR("Failed to load script for properties: {0}", err.what());
				return {};
			}
			scriptClass = result.get<sol::table>();
		}

		std::vector<ScriptProperty> properties;
		properties.reserve(scriptClass.size());

		for (auto& [key, value] : scriptClass)
		{
			std::string name = key.as<std::string>();

			// Skip reserved names: default Ember lifecycle functions, the Base inheritance field, and
			// the engine-set ancestry chain (this table never actually has one - ResolveScriptInheritance
			// isn't invoked here - but excluding it keeps this loop correct if that ever changes)
			if (std::find(DefaultEmberFunctions.begin(), DefaultEmberFunctions.end(), name) != DefaultEmberFunctions.end())
				continue;
			if (name == BaseFieldName || name == BaseChainFieldName)
				continue;

			if (auto property = ParseScriptPropertyValue(GetState(), name, value))
				properties.push_back(std::move(*property));
		}

		MergeInheritedProperties(scriptAsset, scriptClass, properties, visitedBaseChain);
		SortPropertiesByDeclarationOrder(filePath, properties);

		return properties;
	}

	void ScriptEngine::SetTimeout(sol::protected_function callback, float delaySeconds)
	{
		if (!callback.valid())
		{
			EB_CORE_WARN("Timer.SetTimeout ignored an invalid callback.");
			return;
		}

		TimeoutRequest request;
		request.RemainingSeconds = std::max(0.0f, delaySeconds);
		request.Callback = std::move(callback);
		s_Timeouts.push_back(std::move(request));
	}

	void ScriptEngine::UpdateTimeouts(TimeStep delta)
	{
		if (s_Timeouts.empty() || delta.Seconds() <= 0.0f)
			return;

		std::vector<sol::protected_function> dueCallbacks;
		dueCallbacks.reserve(s_Timeouts.size());

		for (auto it = s_Timeouts.begin(); it != s_Timeouts.end();)
		{
			it->RemainingSeconds -= delta.Seconds();
			if (it->RemainingSeconds <= 0.0f)
			{
				dueCallbacks.emplace_back(std::move(it->Callback));
				it = s_Timeouts.erase(it);
			}
			else
			{
				++it;
			}
		}

		for (auto& callback : dueCallbacks)
		{
			sol::protected_function_result result = callback();
			if (!result.valid())
			{
				sol::error err = result;
				EB_CORE_ERROR("Lua Timer.SetTimeout Error: {}", err.what());
			}
		}
	}

	void ScriptEngine::ClearTimeouts()
	{
		s_Timeouts.clear();
	}
}
