#pragma once

#include "Ember/Core/Core.h"
#include "Ember/ECS/Component/Components.h"

#include <unordered_map>
#include <string>
#include <functional>

#include <sol/sol.hpp>

namespace Ember {

	class ScriptRegistry : public SharedResource
	{
	public:
		ScriptRegistry() = default;
		~ScriptRegistry() = default;

		sol::table LoadScript(const std::string& filePath, sol::state& luaState);

		void Clear();

	private:
		std::unordered_map<std::string, sol::table> m_Scripts;
	};

}