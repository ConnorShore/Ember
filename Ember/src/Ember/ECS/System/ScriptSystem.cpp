#include "ebpch.h"
#include "ScriptSystem.h"

#include "Ember/Core/Application.h"
#include "Ember/Input/Input.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Scene/Entity.h"
#include "Ember/Scene/Behavior.h"
#include "Ember/Scene/Scene.h"
#include "Ember/Script/Script.h"
#include "Ember/Script/ScriptEngine.h"

#include <stdexcept>

namespace Ember {

	void ScriptSystem::OnAttach()
	{
		EB_CORE_INFO("ScriptSystem attached!");
	}

	void ScriptSystem::OnDetach()
	{
	}

	void ScriptSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		EB_PROFILE_FUNCTION();

		// Prevents scripts like mouse move from updating if game is paused
		if (delta.IsZero())
			return;

		ScriptEngine::UpdateTimeouts(delta);

		auto view = scene->GetRegistry().ActiveQuery<ScriptComponent>();

		// Snapshot entity IDs because script OnCreate/OnUpdate callbacks can mutate the
		// scene and invalidate query iterators/component references mid-iteration.
		std::vector<EntityID> scriptEntities;
		for (auto entityID : view)
			scriptEntities.push_back(entityID);

		sol::state& luaState = ScriptEngine::GetState();

		for (auto entityID : scriptEntities)
		{
			// Detail scope: per-entity Lua dispatch. See the note in AnimationSystem::OnUpdate —
			// same "bounded zone-name cardinality" tradeoff applies here for many-scripted-entity scenes.
			EB_PROFILE_SCOPE("ScriptSystem::EntityOnUpdate");

			Entity entity{ entityID, scene };
			if (!entity.ContainsComponent<ScriptComponent>())
				continue;

			auto& script = entity.GetComponent<ScriptComponent>();

			if (script.ScriptHandle == Constants::InvalidUUID)
				continue;

			// First frame: load the Lua file, create an instance table, and call OnCreate
			if (!script.Initialized)
			{
				InitializeScriptForEntity(entity);
			}

			// Re-fetch after InitializeScriptForEntity because callbacks can mutate the scene
			// and invalidate previously held component references.
			if (!entity.ContainsComponent<ScriptComponent>())
				continue;

			auto& refreshedScript = entity.GetComponent<ScriptComponent>();

			if (refreshedScript.Initialized && refreshedScript.Instance.valid())
			{
				sol::protected_function onUpdate = refreshedScript.Instance["OnUpdate"];
				if (onUpdate.valid())
				{
					sol::protected_function_result updateResult = onUpdate(refreshedScript.Instance, Entity{ entity }, delta.Seconds());

					if (!updateResult.valid())
					{
						sol::error err = updateResult;
						EB_CORE_ERROR("Lua OnUpdate Error: {0}", err.what());
						throw std::runtime_error(err.what());
					}
				}
			}
		}
	}
	void ScriptSystem::FireTriggerEvent(EntityID receiverID, EntityID otherID, TriggerEventType type, Scene* scene)
	{
		Entity receiver = { receiverID, scene };
		if (!receiver.ContainsComponent<ScriptComponent>())
			return;

		auto& script = receiver.GetComponent<ScriptComponent>();

		// Get the function name based on the event type
		const char* functionName = "";
		if (type == TriggerEventType::OverlapEnter)
			functionName = "OnOverlapTriggerEnter";
		if (type == TriggerEventType::OverlapStay)
			functionName = "OnOverlapTriggerStay";													
		if (type == TriggerEventType::OverlapExit)
			functionName = "OnOverlapTriggerExit";

		// Execute the Lua function if it exists
		sol::function triggerFunc = script.Instance[functionName];
		if (triggerFunc.valid())
		{
			Entity otherEntity = { otherID, scene };
			sol::protected_function_result result = triggerFunc(script.Instance, Entity{receiver}, Entity{otherEntity});
			if (!result.valid())
			{
				sol::error err = result;
				EB_CORE_ERROR("Lua {} Error: {}", functionName, err.what());
				throw std::runtime_error(err.what());
			}
		}
	}

	void ScriptSystem::FireAnimationEvent(EntityID entityID, const std::string& eventName, Scene* scene)
	{
		Entity receiver = { entityID, scene };
		if (!receiver.ContainsComponent<ScriptComponent>())
			return;

		auto& script = receiver.GetComponent<ScriptComponent>();
		const char* functionName = "OnAnimationEvent";

		// Execute the Lua function if it exists
		sol::function eventFunc = script.Instance[functionName];
		if (eventFunc.valid())
		{
			sol::protected_function_result result = eventFunc(script.Instance, eventName);
			if (!result.valid())
			{
				sol::error err = result;
				EB_CORE_ERROR("Lua {} Error: {}", functionName, err.what());
				throw std::runtime_error(err.what());
			}
		}
	}

	void ScriptSystem::InitializeScriptForEntity(Entity entity)
	{
		EB_PROFILE_FUNCTION();

		if (!entity.ContainsComponent<ScriptComponent>())
		{
			EB_CORE_ERROR("Attempted to initialize script for entity '{}' but it doesn't have a ScriptComponent!", entity.GetName());
			return;
		}

		sol::state& luaState = ScriptEngine::GetState();
		auto& script = entity.GetComponent<ScriptComponent>();
		auto scriptAsset = Application::Instance().GetAssetManager().GetAsset<Script>(script.ScriptHandle);
		if (scriptAsset)
		{
			std::string filepath = scriptAsset->GetFilePath();

			// Scoped so this stack-based result is released BEFORE any nested script_file call
			// below. ResolveScriptInheritance and GetScriptProperties both re-enter script_file
			// once per Base level, and sol2 cleans a result up with lua_remove (which shifts every
			// slot above it) - holding this one open across that nesting left the stack mis-indexed.
			sol::table scriptClass;
			{
				sol::protected_function_result result = luaState.script_file(filepath);
				if (!result.valid())
				{
					sol::error err = result;
					EB_CORE_ERROR("Failed to load script '{}': {}", filepath, err.what());
					throw std::runtime_error(err.what());
				}
				scriptClass = result.get<sol::table>();
			}

			{
				scriptClass["__name"] = scriptAsset->GetName();

				// Chain scriptClass's own metatable to any Base script declared, so methods not
				// defined on this script fall back to its parent(s) before we build the per-entity
				// instance below.
				ScriptEngine::ResolveScriptInheritance(scriptClass, scriptAsset->GetName());

				// Create a per-entity Lua table that inherits from the script class via __index
				script.Instance = luaState.create_table();
				script.Instance[sol::metatable_key] = luaState.create_table_with("__index", scriptClass);

				// Seed instance fields with script defaults so enum/table-backed properties
				// resolve to concrete values even when no user override has been authored.
				const auto& cachedProperties = scriptAsset->GetExposedProperties();
				if (!cachedProperties.empty())
				{
					for (const auto& property : cachedProperties)
					{
						script.Instance[property.Name] = ScriptEngine::ScriptPropertyValueToLua(luaState, property.Value);
					}
				}
				else
				{
					auto parsedProperties = ScriptEngine::GetScriptProperties(scriptAsset);
					for (const auto& property : parsedProperties)
					{
						script.Instance[property.Name] = ScriptEngine::ScriptPropertyValueToLua(luaState, property.Value);
					}
					scriptAsset->SetExposedProperties(parsedProperties);
				}

				// Inject user property overrides from the component
				for (const auto& [name, overrideProp] : script.UserPropertyOverrides)
				{
					// Shove the C++ override directly into the Lua table
					sol::object objValue = ScriptEngine::ScriptPropertyValueToLua(luaState, overrideProp.Value);
					script.Instance[name] = objValue;
				}

				// Call OnCreate
				sol::protected_function onCreate = scriptClass["OnCreate"];
				if (onCreate.valid())
				{
					// Pass Entity as a temporary rvalue so sol2 creates an owned userdata copy
					// rather than a reference to the loop-local stack variable.
					sol::protected_function_result createResult = onCreate(script.Instance, Entity{ entity });
					if (!createResult.valid())
					{
						sol::error err = createResult;
						EB_CORE_ERROR("Lua OnCreate Error in '{}': {}", filepath, err.what());
						throw std::runtime_error(err.what());
					}
				}
			}
		}
		else
		{
			EB_CORE_ERROR("ScriptSystem: Invalid ScriptHandle ID");
			throw std::runtime_error("ScriptSystem: Invalid ScriptHandle ID");
		}

		// Re-fetch: OnCreate can attach components (spawning a prefab, duplicating an entity), which
		// grows the packed ScriptComponent storage and dangles `script`. Marked unconditionally to
		// prevent error spam on every frame.
		if (entity.ContainsComponent<ScriptComponent>())
			entity.GetComponent<ScriptComponent>().Initialized = true;
	}

}