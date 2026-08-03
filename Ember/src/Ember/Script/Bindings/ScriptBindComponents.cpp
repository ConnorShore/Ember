#include "ebpch.h"
#include "ScriptBindComponents.h"

// The per-domain component binders are defined in their ScriptBindComponents*.cpp siblings.
// There is deliberately no bind-everything helper here: the stateless binders must run only once
// per Lua state, while BindAIComponents is rebound per scene. ScriptEngine::BindAPI owns that
// split - see s_StatelessBindingsRegistered in ScriptEngine.cpp.
