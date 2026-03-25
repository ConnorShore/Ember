#pragma once

#include <sol/sol.hpp>

namespace Ember {

	class ScriptEngine
	{
	public:

		static void Init();
		static void Shutdown();

		static void OnRuntimeStart();
		static void OnRuntimeStop();

		static sol::state& GetState();

	private:
		static void BindAPI();
		static void BindInput();
		static void BindMath();
		static void BindComponents();
	};

}