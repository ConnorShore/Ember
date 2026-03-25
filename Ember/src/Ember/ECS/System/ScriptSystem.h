#pragma once

#include "System.h"
#include "Ember/Asset/ScriptRegistry.h"

#include <sol/sol.hpp>

namespace Ember {

	class Scene;

	class ScriptSystem : public System
	{
	public:
		ScriptSystem() : m_ScriptRegistry(SharedPtr<ScriptRegistry>::Create()) {}
		virtual ~ScriptSystem() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;

	private:
		void BindAPI();

		// Helpers
		void BindInput();
		void BindMath();
		void BindComponents();

	private:
		sol::state m_LuaState;
		SharedPtr<ScriptRegistry> m_ScriptRegistry;
	};

}