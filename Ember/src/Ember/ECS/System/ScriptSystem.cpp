#include "ebpch.h"
#include "ScriptSystem.h"

#include "Ember/ECS/Component/Components.h"
#include "Ember/Scene/Entity.h"
#include "Ember/Scene/Behavior.h"
#include "Ember/Scene/Scene.h"

namespace Ember {

	void ScriptSystem::OnAttach()
	{
	}

	void ScriptSystem::OnDetach()
	{
	}

	void ScriptSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		auto view = scene->GetRegistry().Query<ScriptComponent>();
		for (auto entity : view)
		{
			Entity handle(entity, scene);
			auto& script = scene->GetRegistry().GetComponent<ScriptComponent>(entity);

			if (!script.Initalized)
			{
				if (script.OnInitFunc)
				{
					// Behavior class scripts bound
					script.Instance = script.OnInitFunc();
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