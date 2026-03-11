#include "ebpch.h"
#include "ScriptSystem.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/Scene/Entity.h"
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
			Entity handle(entity, m_SceneHandle);
			auto& script = registry->GetComponent<ScriptComponent>(entity);

			if (!script.Initalized)
			{
				if (script.OnInitScript)
				{
					// Behavior class scripts bound
					script.Instance = script.OnInitScript();
					script.Instance->m_EntityHandle = handle;
					script.Instance->OnCreate();
				}
				else if (script.OnCreate)
				{
					// Inline lambda scripts bound
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