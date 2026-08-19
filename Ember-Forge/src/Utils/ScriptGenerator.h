#pragma once

#include <string>

namespace Ember {

	class Script;

	class ScriptGenerator
	{
	public:
		static SharedPtr<Script> GenerateStandardScriptTemplate(const std::string& scriptName, const std::string& filepath);
		static SharedPtr<Script> GenerateButtonScriptTemplate(const std::string& scriptName, const std::string& filepath);
	};

}