#include "ebpch.h"
#include "ScriptSystem.h"

#include "Ember/Input/Input.h"
#include "Ember/ECS/Component/Components.h"
#include "Ember/Scene/Entity.h"
#include "Ember/Scene/Behavior.h"
#include "Ember/Scene/Scene.h"

namespace Ember {

	void ScriptSystem::OnAttach()
	{
		m_LuaState.open_libraries(sol::lib::base, sol::lib::math, sol::lib::os, sol::lib::string);
		BindAPI();
		EB_CORE_INFO("ScriptSystem attached!");
	}

	void ScriptSystem::OnDetach()
	{
	}

	void ScriptSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		auto view = scene->GetRegistry().Query<ScriptComponent>();

		for (auto entityID : view)
		{
			Entity entity{ entityID, scene };
			auto& script = entity.GetComponent<ScriptComponent>();

			if (script.FilePath.empty())
				continue;

			if (!script.Initialized)
			{
				// Load the script file. It returns the Lua "class" table.
				sol::protected_function_result result = m_LuaState.script_file(script.FilePath);

				if (result.valid())
				{
					sol::table scriptClass = result;

					// Object-Oriented Lua: Create a new empty table for this specific entity, 
					// and set its metatable to the scriptClass so it inherits the functions.
					script.Instance = m_LuaState.create_table();
					script.Instance[sol::metatable_key] = m_LuaState.create_table_with("__index", scriptClass);

					// Call OnCreate (Lua uses ':' syntax, so we must explicitly pass 'self' as the first arg)
					sol::protected_function onCreate = scriptClass["OnCreate"];
					if (onCreate.valid())
					{
						// Call it: OnCreate(self, entity)
						sol::protected_function_result createResult = onCreate(script.Instance, entity);
						if (!createResult.valid())
						{
							sol::error err = createResult;
							EB_CORE_ERROR("Lua OnCreate Error: {0}", err.what());
						}
					}

					script.Initialized = true;
				}
				else
				{
					sol::error err = result;
					EB_CORE_ERROR("Failed to load Lua Script '{0}': {1}", script.FilePath, err.what());

					// Mark as initialized anyway so we don't spam the console 60 times a second
					script.Initialized = true;
				}
			}

			// 2. UPDATE LOOP (Runs every frame)
			if (script.Initialized && script.Instance.valid())
			{
				sol::protected_function onUpdate = script.Instance["OnUpdate"];
				if (onUpdate.valid())
				{
					// Call it: OnUpdate(self, delta)
					sol::protected_function_result updateResult = onUpdate(script.Instance, delta.Seconds());

					if (!updateResult.valid())
					{
						sol::error err = updateResult;
						EB_CORE_ERROR("Lua OnUpdate Error: {0}", err.what());
					}
				}
			}
		}
	}

	void ScriptSystem::BindAPI()
	{
		m_LuaState.new_usertype<Vector3f>("Vector3f",
			"x", &Vector3f::x,
			"y", &Vector3f::y,
			"z", &Vector3f::z
		);

		m_LuaState.new_usertype<TransformComponent>("TransformComponent",
			"Position", &TransformComponent::Position,
			"Rotation", &TransformComponent::Rotation,
			"Scale", &TransformComponent::Scale
		);

		m_LuaState.new_usertype<Entity>("Entity",
			"GetTransform", [](Entity& e) -> TransformComponent& { return e.GetComponent<TransformComponent>(); },
			"GetName", & Entity::GetName,
			"GetUUID", & Entity::GetUUID
		);

		// Expose global engine functions
		m_LuaState.set_function("IsKeyPressed", &Input::IsKeyPressed);
	}

}