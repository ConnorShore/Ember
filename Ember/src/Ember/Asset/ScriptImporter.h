#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Asset/Script.h"

#include <sol/sol.hpp>

namespace Ember {

	class ScriptImporter
	{
	public:

		static SharedPtr<Script> LoadScript(const std::string& filePath, sol::state& luaState);

	};

}