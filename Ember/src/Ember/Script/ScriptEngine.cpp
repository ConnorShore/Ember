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

    void ScriptEngine::Init()
    {
		// Create the state immediately so the Editor can parse scripts!
		s_LuaState = new sol::state();
		s_LuaState->open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
		ClearTimeouts();

		// Bind core/math helpers so scripts using UUID/ref helpers or Vector3f defaults can be parsed
		BindCore(*s_LuaState);
		BindMath(*s_LuaState);

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
		BindCore(*s_LuaState);
		BindScene(*s_LuaState, scene);
		BindEntity(*s_LuaState);
		BindInput(*s_LuaState);
		BindMath(*s_LuaState);
		BindPhysics(*s_LuaState, scene);
		BindAllComponents(*s_LuaState, scene);
		BindAssets(*s_LuaState);
		BindAudio(*s_LuaState, scene);
		BindSaveGame(*s_LuaState);
		BindDebugDraw(*s_LuaState);
	}

    // Creates a fresh Lua VM for each play session so scripts start with clean state
	void ScriptEngine::OnRuntimeStart(Scene* scene)
	{
		// wipe editor state and create fresh one for runtime
		ClearTimeouts();
		delete s_LuaState;
		s_LuaState = new sol::state();
		s_LuaState->open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);

		// 3. Bind the Engine API to this fresh runtime state
		BindAPI(scene);
	}

	void ScriptEngine::OnRuntimeStop(Scene* scene)
	{
		// Null out all sol::table instances in ScriptComponents before destroying the Lua state.
		// The runtime scene is still alive at this point; when it is later destroyed its registry
		// destructs every ScriptComponent, and a sol::table destructor that touches a dead lua_State
		// causes an access violation inside lua's gettable. Resetting them here is safe because
		// scripts are no longer running.
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
		s_LuaState = new sol::state();
		s_LuaState->open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);

		// Bind core/math helpers so scripts using UUID/ref helpers or Vector3f defaults can be parsed
		BindCore(*s_LuaState);
		BindMath(*s_LuaState);
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

	std::vector<ScriptProperty> ScriptEngine::GetScriptProperties(const SharedPtr<Script>& scriptAsset)
	{
		// Evaluate the script to get the base table
		std::string filePath = scriptAsset->GetFilePath();
		sol::protected_function_result result = GetState().script_file(filePath);
        if (!result.valid())
        {
            sol::error err = result;
            EB_CORE_ERROR("Failed to load script for properties: {0}", err.what());
            return {};
		}

        sol::table scriptClass = result;

		std::vector<ScriptProperty> properties;
		properties.reserve(scriptClass.size());

        for (auto& [key, value] : scriptClass)
		{
			std::string name = key.as<std::string>();

			// Check if the property is a default Ember function, skip if so
            if (std::find(DefaultEmberFunctions.begin(), DefaultEmberFunctions.end(), name) != DefaultEmberFunctions.end())
                continue;

			ScriptPropertyType type = ScriptPropertyType::Unknown;
			ScriptPropertyValue val;
			ScriptReferenceKind referenceKind = ScriptReferenceKind::None;
			std::vector<std::pair<std::string, int>> enumOptions;
            switch (value.get_type())
            {
                case sol::type::number:
				{
					// Push value to top of stack so we can inspect it
					value.push();
					bool isInteger = lua_isinteger(GetState(), -1);
					lua_pop(GetState(), 1);	// Pop it off

					if (isInteger)
					{
						type = ScriptPropertyType::Int;
						val = value.as<int>();
					}
					else
					{
						type = ScriptPropertyType::Float;
						val = value.as<float>();
					}
					break;
				}
                case sol::type::string:
					type = ScriptPropertyType::String;
					val = value.as<std::string>();
                    break;
                case sol::type::boolean:
					type = ScriptPropertyType::Bool;
					val = value.as<bool>();
                    break;
				case sol::type::userdata:
				{
					// Detect Vector3f userdata values exposed as script properties
					// e.g. MyScript.MyVec = Vector3f.new(1.0, 2.0, 3.0)
					if (value.is<Vector3f>())
					{
						type = ScriptPropertyType::Vector3f;
						val = value.as<Vector3f>();
					}
					else
					{
						EB_CORE_WARN("Unsupported userdata script property type for '{}'", name);
						continue;
					}
					break;
				}
				case sol::type::table:
				{
					sol::table tableValue = value.as<sol::table>();
					sol::optional<std::string> propertyType = tableValue["__ember_property_type"];
					if (propertyType && propertyType.value() == "Reference")
					{
						sol::optional<std::string> kindName = tableValue["Kind"];
						referenceKind = ScriptReferenceKindFromString(kindName.value_or("Asset"));

						uint64_t rawValue = (uint64_t)Constants::InvalidUUID;
						sol::optional<uint64_t> numericValue = tableValue["Value"];
						if (numericValue)
							rawValue = numericValue.value();

						type = referenceKind == ScriptReferenceKind::Entity
							? ScriptPropertyType::EntityRef
							: ScriptPropertyType::AssetRef;
						val = UUID(rawValue);
						break;
					}

					// Treat string-keyed tables of integers as enums.
					// e.g. PickupType = { Ammo = 1, Health = 2, Points = 3 }
					sol::table enumTable = tableValue;
					bool isEnum = !enumTable.empty();
					for (auto& [enumKey, enumValue] : enumTable)
					{
						if (enumKey.get_type() != sol::type::string ||
							enumValue.get_type() != sol::type::number)
						{
							isEnum = false;
							break;
						}

						enumValue.push();
						bool isInteger = lua_isinteger(GetState(), -1);
						lua_pop(GetState(), 1);
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
						continue;
					}

					// Keep declaration order stable so the editor combo matches the script
					std::sort(enumOptions.begin(), enumOptions.end(),
						[](const auto& a, const auto& b) { return a.second < b.second; });

					type = ScriptPropertyType::Enum;
					val = enumOptions.front().second; // default to first option
					break;
				}
                default:
                    EB_CORE_WARN("Unsupported script property type for '{}'", name);
                    continue; // Skip unsupported types
			}

				if (type == ScriptPropertyType::Enum)
					properties.emplace_back(name, val, type, std::move(enumOptions));
				else if (type == ScriptPropertyType::EntityRef || type == ScriptPropertyType::AssetRef)
					properties.emplace_back(name, val, type, referenceKind);
				else
					properties.emplace_back(name, val, type);
				}

				// Sort properties by their declaration order in the script file so the
				// editor always displays them in the same order they appear in the source.
				{
					std::ifstream scriptFile(filePath);
					if (scriptFile.is_open())
					{
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
								if (pos != std::string::npos)
								{
									auto after = pos + prop.Name.size();
									// skip whitespace
									while (after < line.size() && line[after] == ' ') ++after;
									if (after < line.size() && line[after] == '=' &&
										(after + 1 >= line.size() || line[after + 1] != '='))
									{
										lineNumbers[prop.Name] = lineNum;
									}
								}
							}
						}

						std::sort(properties.begin(), properties.end(),
							[&lineNumbers](const ScriptProperty& a, const ScriptProperty& b)
							{
								int la = lineNumbers.count(a.Name) ? lineNumbers[a.Name] : INT_MAX;
								int lb = lineNumbers.count(b.Name) ? lineNumbers[b.Name] : INT_MAX;
								return la < lb;
							});
					}
				}

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