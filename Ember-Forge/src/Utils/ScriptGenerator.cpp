#include "efpch.h"
#include "ScriptGenerator.h"

#include "Ember/Core/Application.h"

#include <fstream>

namespace Ember {

	SharedPtr<Script> ScriptGenerator::GenerateStandardScriptTemplate(const std::string& scriptName, const std::string& filepath)
	{
		// Script template
		std::ofstream newScriptFile(filepath);
		newScriptFile << "local " << scriptName << " = {}\n\n";
		newScriptFile << "-- Expose properties to the editor by adding them to this table. For Example:\n";
		newScriptFile << "-- " << scriptName << ".MyExampleVar = 10\n\n";
		newScriptFile << "function " << scriptName << ":OnCreate(entity)\n\nend\n\n";
		newScriptFile << "function " << scriptName << ":OnUpdate(entity, delta)\n\nend\n\n";
		newScriptFile << "return " << scriptName;
		newScriptFile.close();

		// Load it
		auto& assetManager = Application::Instance().GetAssetManager();
		auto scriptAsset = assetManager.Load<Script>(filepath, false);
		return scriptAsset;
	}

	SharedPtr<Script> ScriptGenerator::GenerateButtonScriptTemplate(const std::string& scriptName, const std::string& filepath)
	{
		// Script template
		std::ofstream newScriptFile(filepath);
		newScriptFile << "local " << scriptName << " = {}\n\n";
		newScriptFile << "-- Expose properties to the editor by adding them to this table. For Example:\n";
		newScriptFile << "-- " << scriptName << ".MyExampleVar = 10\n\n";
		newScriptFile << "function " << scriptName << ":OnCreate(entity)\n\nend\n\n";
		newScriptFile << "function " << scriptName << ":OnUpdate(entity, delta)\n\nend\n\n";
		newScriptFile << "function " << scriptName << ":OnClick(entity)\n\nend\n\n";
		newScriptFile << "function " << scriptName << ":OnHoverEnter(entity)\n\nend\n\n";
		newScriptFile << "function " << scriptName << ":OnHoverExit(entity)\n\nend\n\n";
		newScriptFile << "return " << scriptName;
		newScriptFile.close();

		// Load it
		auto& assetManager = Application::Instance().GetAssetManager();
		auto scriptAsset = assetManager.Load<Script>(filepath, false);
		return scriptAsset;
	}

}