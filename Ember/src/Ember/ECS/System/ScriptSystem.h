#pragma once

#include "System.h"

#include <sol/sol.hpp>

namespace Ember {

	class Scene;

	class ScriptSystem : public System
	{
	public:
		ScriptSystem() = default;
		virtual ~ScriptSystem() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;

	private:
		void BindAPI();

	private:
		sol::state m_LuaState;
	};

}