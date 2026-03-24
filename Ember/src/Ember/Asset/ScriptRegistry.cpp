#include "ebpch.h"
#include "ScriptRegistry.h"

namespace Ember {

	std::unordered_map<std::string, ScriptRegistry::ScriptFunctions> ScriptRegistry::s_ScriptRegistry;

	bool ScriptRegistry::GetScriptFunctions(const std::string& className, ScriptFunctions& outFunctions)
	{
		if (s_ScriptRegistry.find(className) != s_ScriptRegistry.end())
		{
			outFunctions = s_ScriptRegistry[className];
			return true;
		}

		return false;
	}

	bool ScriptRegistry::ScriptExists(const std::string& className)
	{
		return s_ScriptRegistry.find(className) != s_ScriptRegistry.end();
	}
}