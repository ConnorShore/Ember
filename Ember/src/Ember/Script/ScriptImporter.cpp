#include "ebpch.h"
#include "ScriptImporter.h"
#include "ScriptEngine.h"

namespace Ember {

	SharedPtr<Script> ScriptImporter::LoadScript(UUID uuid, const std::string& name, const std::string& filePath)
	{
		if (!std::filesystem::exists(filePath))
		{
			EB_CORE_ERROR("ScriptImporter: File does not exist at path '{0}'", filePath);
			return nullptr;
		}

		sol::state tempState;
		sol::load_result result = tempState.load_file(filePath);

		if (result.valid())
		{
			EB_CORE_TRACE("ScriptImporter: Successfully verified syntax for '{0}'", filePath);

			// Load default exposed script properties
			auto scriptAsset = SharedPtr<Script>::Create(uuid, name, filePath);
			scriptAsset->SetExposedProperties(ScriptEngine::GetScriptProperties(scriptAsset));

			return scriptAsset;
		}
		else
		{
			sol::error err = result;
			EB_CORE_ERROR("ScriptImporter: Syntax Error in '{0}'\n{1}", filePath, err.what());
			return nullptr;
		}
	}

}