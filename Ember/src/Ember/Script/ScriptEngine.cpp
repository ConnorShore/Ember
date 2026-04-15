#include "ebpch.h"
#include "ScriptEngine.h"

#include "Bindings/ScriptBindCore.h"
#include "Bindings/ScriptBindEntity.h"
#include "Bindings/ScriptBindInput.h"
#include "Bindings/ScriptBindMath.h"
#include "Bindings/ScriptBindPhysics.h"
#include "Bindings/ScriptBindComponents.h"
#include "Bindings/ScriptBindAssets.h"

namespace Ember {

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
            OnRuntimeStop();

        EB_CORE_INFO("Shutdown Script Engine...");
    }

    // Creates a fresh Lua VM for each play session so scripts start with clean state
    void ScriptEngine::OnRuntimeStart(Scene* scene)
    {
        s_LuaState = new sol::state();
        s_LuaState->open_libraries(sol::lib::base, sol::lib::math, sol::lib::os, sol::lib::string);
        BindAPI(scene);

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

    void ScriptEngine::BindAPI(Scene* scene)
    {
        BindCore(*s_LuaState);
        BindEntity(*s_LuaState);
        BindInput(*s_LuaState);
        BindMath(*s_LuaState);
        BindPhysics(*s_LuaState, scene);
        BindComponents(*s_LuaState);
        BindAssets(*s_LuaState);
    }

}