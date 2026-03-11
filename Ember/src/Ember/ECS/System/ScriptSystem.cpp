#include "ebpch.h"
#include "ScriptSystem.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/Scene/SceneEntity.h"
#include "Ember/Scene/Behavior.h"

namespace Ember {

	void ScriptSystem::OnAttach(Registry* registry)
	{
	}

	void ScriptSystem::OnDetach(Registry* registry)
	{
	}

	void ScriptSystem::OnUpdate(TimeStep delta, Registry* registry)
	{
		auto view = registry->Query<ScriptComponent>();
		for (auto entity : view)
		{
			SceneEntity handle(entity, m_SceneHandle);
			auto& script = registry->GetComponent<ScriptComponent>(entity);

			// Run Creation scripts if not yet initialized
			if (!script.Initalized)
			{
				if (script.CreateScript)
				{
					// They bound a script class
					script.Instance = script.CreateScript();
					script.Instance->m_SceneEntityHandle = handle;
					script.Instance->OnCreate();
				}
				else if (script.OnCreate)
				{
					script.OnCreate(handle);
				}

				script.Initalized = true;
			}

			// Update scripts
			if (script.Instance)
			{
				script.Instance->OnUpdate(delta);
			}
			else if (script.OnUpdate)
			{
				script.OnUpdate(handle, delta);
			}

		}
	}

}