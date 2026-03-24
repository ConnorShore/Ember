#pragma once

#include "Ember/Core/Core.h"
#include "Ember/ECS/Component/Components.h"

#include <unordered_map>
#include <string>
#include <functional>

namespace Ember {

	class ScriptRegistry
	{
	public:
        using InstantiateFn = Behavior* (*)();
		using DestroyFn = void (*)(ScriptComponent*);

		struct ScriptFunctions
		{
			InstantiateFn Instantiate;
			DestroyFn Destroy;
		};

		template<typename T>
		static void RegisterScript(const std::string& className)
		{
			s_ScriptRegistry[className] = {
				[]() { return static_cast<Behavior*>(new T()); },
				[](ScriptComponent* sc) { delete sc->Instance; sc->Instance = nullptr; }
			};
		}

		// Retrieve the functions when deserializing
		static bool GetScriptFunctions(const std::string& className, ScriptFunctions& outFunctions);
		static bool ScriptExists(const std::string& className);

	private:
		static std::unordered_map<std::string, ScriptFunctions> s_ScriptRegistry;
	};

}