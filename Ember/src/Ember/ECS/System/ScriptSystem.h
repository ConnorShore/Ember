#pragma once

#include "System.h"
#include "Ember/Script/ScriptImporter.h"
#include "Ember/Physics/OverlapTriggerEventData.h"

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

		static void FireTriggerEvent(EntityID entityA, EntityID entityB, TriggerEventType eventType, Scene* scene);

	private:
		sol::state m_LuaState;
	};

}