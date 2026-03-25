#include "ebpch.h"
#include "ScriptImporter.h"

namespace Ember {

	SharedPtr<Script> ScriptImporter::LoadScript(const std::string& filePath, sol::state& luaState)
	{
		sol::protected_function_result result = luaState.script_file(filePath);
		if (result.valid())
		{
			sol::table scriptClass = result;
			EB_CORE_TRACE("Loaded script {}", filePath);
			return SharedPtr<Script>::Create(filePath, scriptClass);
		}
		else
		{
			sol::error err = result;
			EB_CORE_ERROR("Failed to load Lua Script '{0}': {1}", filePath, err.what());
		}

		return nullptr;
	}

}