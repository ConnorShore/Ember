#pragma once

#include <sol/sol.hpp>

namespace Ember {

	class Scene;

	class ScriptEngine
	{
	public:
		static void Init();
		static void Shutdown();

		static void OnRuntimeStart(Scene* scene);
		static void OnRuntimeStop();

		static sol::state& GetState();

	private:
		static void BindAPI(Scene* scene);
	};

}