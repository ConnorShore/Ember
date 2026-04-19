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
		auto view = scene->GetRegistry().Query<ScriptComponent>();

		sol::state& luaState = ScriptEngine::GetState();

		for (auto entityID : view)
		{
			Entity entity{ entityID, scene };
			auto& script = entity.GetComponent<ScriptComponent>();

			if (script.ScriptHandle == Constants::InvalidUUID)
				continue;

			// First frame: load the Lua file, create an instance table, and call OnCreate
			if (!script.Initialized)
			{
				auto scriptAsset = Application::Instance().GetAssetManager().GetAsset<Script>(script.ScriptHandle);
				if (scriptAsset)
				{
					std::string filepath = scriptAsset->GetFilePath();
					sol::protected_function_result result = luaState.script_file(filepath);
					if (result.valid())
					{
						sol::table scriptClass = result;

						// Create a per-entity Lua table that inherits from the script class via __index
						script.Instance = luaState.create_table();
						script.Instance[sol::metatable_key] = luaState.create_table_with("__index", scriptClass);

						// Call OnCreate
						sol::protected_function onCreate = scriptClass["OnCreate"];
						if (onCreate.valid())
						{
							sol::protected_function_result createResult = onCreate(script.Instance, entity);
							if (!createResult.valid())
							{
								sol::error err = createResult;
								EB_CORE_ERROR("Lua OnCreate Error in '{}': {}", filepath, err.what());
							}
						}
					}
					else
					{
						sol::error err = result;
						EB_CORE_ERROR("Failed to load script '{}': {}", filepath, err.what());
					}
				}
				else
				{
					EB_CORE_ERROR("ScriptSystem: Invalid ScriptHandle ID");
				}

				// Mark as initialized unconditionally to prevent error spam on every frame
				script.Initialized = true;
			}

			if (script.Initialized && script.Instance.valid())
			{
				sol::protected_function onUpdate = script.Instance["OnUpdate"];
				if (onUpdate.valid())
				{
					sol::protected_function_result updateResult = onUpdate(script.Instance, entity, delta.Seconds());

					if (!updateResult.valid())
					{
						sol::error err = updateResult;
						EB_CORE_ERROR("Lua OnUpdate Error: {0}", err.what());
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
			sol::protected_function_result result = triggerFunc(script.Instance, receiver, otherEntity);
			if (!result.valid())
			{
				sol::error err = result;
				EB_CORE_ERROR("Lua {} Error: {}", functionName, err.what());
			}
		}
	}
}