#pragma once

#include "System.h"
#include "Ember/Script/ScriptImporter.h"
#include "Ember/Physics/OverlapTriggerEventData.h"

#include <sol/sol.hpp>

namespace Ember {

	class Scene;
	class Entity;

	class ScriptSystem : public System
	{
	public:
		ScriptSystem() = default;
		virtual ~ScriptSystem() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(TimeStep delta, Scene* scene) override;

		void InitializeScriptForEntity(Entity entity);

		static void FireTriggerEvent(EntityID entityA, EntityID entityB, TriggerEventType eventType, Scene* scene);
		static void FireAnimationEvent(EntityID entityID, const std::string& eventName, Scene* scene);

		// Dispatches a UI lifecycle hook. Unlike the trigger events this can run before scripts are
		// initialised, because UIInputSystem ticks ahead of ScriptSystem.
		static void FireUIEvent(EntityID entityID, const char* functionName, Scene* scene);
		static void FireUIEvent(EntityID entityID, const char* functionName, Scene* scene, bool argument);
	};

}