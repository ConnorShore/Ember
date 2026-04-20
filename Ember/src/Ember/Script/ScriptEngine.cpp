#include "ebpch.h"
#include "ScriptEngine.h"

#include "Bindings/ScriptBindCore.h"
#include "Bindings/ScriptBindEntity.h"
#include "Bindings/ScriptBindInput.h"
#include "Bindings/ScriptBindMath.h"
#include "Bindings/ScriptBindPhysics.h"
#include "Bindings/ScriptBindComponents.h"
#include "Bindings/ScriptBindAssets.h"
#include "Bindings/ScriptBindScene.h"

#include "Ember/Core/Core.h"
#include "Ember/ECS/Component/Components.h"

#include <algorithm>

namespace Ember {

    //////////////////////////////////////////////////////////////////////////
    // Script Engine
    //////////////////////////////////////////////////////////////////////////

    static sol::state* s_LuaState = nullptr;

    void ScriptEngine::Init()
    {
		// Create the state immediately so the Editor can parse scripts!
		s_LuaState = new sol::state();
		s_LuaState->open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);

		EB_CORE_INFO("ScriptEngine Initialized (Editor State)");
    }

    void ScriptEngine::Shutdown()
	{
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
		BindComponents(*s_LuaState);
		BindAssets(*s_LuaState);
	}

    // Creates a fresh Lua VM for each play session so scripts start with clean state
	void ScriptEngine::OnRuntimeStart(Scene* scene)
	{
		// wipe editor state and create fresh one for runtime
		delete s_LuaState;
		s_LuaState = new sol::state();
		s_LuaState->open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);

		// 3. Bind the Engine API to this fresh runtime state
		BindAPI(scene);
	}

	void ScriptEngine::OnRuntimeStop()
	{
		// Wipe runtime state and create fresh one for the editor
		delete s_LuaState;
		s_LuaState = new sol::state();
		s_LuaState->open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
	}

    sol::state& ScriptEngine::GetState()
    {
        EB_CORE_ASSERT(s_LuaState, "Attempted to access Lua State while it is dead! Are you in Play Mode?");
        return *s_LuaState;
    }

	std::unordered_map<std::string, ScriptProperty> ScriptEngine::GetScriptProperties(const SharedPtr<Script>& scriptAsset)
	{
		// Evaluate the script to get the base table
		sol::protected_function_result result = GetState().script_file(scriptAsset->GetFilePath());
        if (!result.valid())
        {
            sol::error err = result;
            EB_CORE_ERROR("Failed to load script for properties: {0}", err.what());
            return {};
		}

        sol::table scriptClass = result;

		std::unordered_map<std::string, ScriptProperty> properties;
		properties.reserve(scriptClass.size());

        for (auto& [key, value] : scriptClass)
		{
			std::string name = key.as<std::string>();

			// Check if the property is a default Ember function, skip if so
            if (std::find(DefaultEmberFunctions.begin(), DefaultEmberFunctions.end(), name) != DefaultEmberFunctions.end())
                continue;

			ScriptPropertyType type = ScriptPropertyType::Unknown;
			ScriptPropertyValue val;
            switch (value.get_type())
            {
                case sol::type::number:
                    type = ScriptPropertyType::Number;
					val = value.as<float>();
                    break;
                case sol::type::string:
					type = ScriptPropertyType::String;
					val = value.as<std::string>();
                    break;
                case sol::type::boolean:
					type = ScriptPropertyType::Bool;
					val = value.as<bool>();
                    break;
                default:
                    EB_CORE_WARN("Unsupported script property type for '{}'", name);
                    continue; // Skip unsupported types
			}

			properties[name] = { name, val, type };
		}

        return properties;
	}
}