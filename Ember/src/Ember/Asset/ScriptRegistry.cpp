#include "ebpch.h"
#include "ScriptRegistry.h"

namespace Ember {

	sol::table ScriptRegistry::LoadScript(const std::string& filePath, sol::state& luaState)
	{
		auto it = m_Scripts.find(filePath);
		if (it != m_Scripts.end())
			return it->second;


		sol::protected_function_result result = luaState.script_file(filePath);
		if (result.valid())
		{
			sol::table scriptClass = result;
			m_Scripts[filePath] = scriptClass;
			EB_CORE_TRACE("Loaded script {}", filePath);
			return scriptClass;
		}
		else
		{
			sol::error err = result;
			EB_CORE_ERROR("Failed to load Lua Script '{0}': {1}", filePath, err.what());
		}

		return sol::table();
	}

	void ScriptRegistry::Clear()
	{
		m_Scripts.clear();
	}

}