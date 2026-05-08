#pragma once

#include "WindowConfig.h"

#include <string>
#include <filesystem>

namespace Ember {

	struct ApplicationSpecification
	{
		std::string Name = "Ember App";
		WindowConfig WindowSpecification = {};

		std::filesystem::path EngineAssetDir = "EmberCore";
		std::filesystem::path ProjectAssetDir = "GameData";

		int CommandLineArgsCount = 0;
		char** CommandLineArgs = nullptr;
	};

}