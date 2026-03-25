#pragma once

#include "Ember/Core/Core.h"
#include "Ember/Script/Script.h"

#include <sol/sol.hpp>

namespace Ember {

	class ScriptImporter
	{
	public:
		static SharedPtr<Script> LoadScript(UUID uuid, const std::string& name, const std::string& filePath);
	};

}