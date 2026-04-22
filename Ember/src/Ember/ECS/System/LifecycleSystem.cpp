#include "ebpch.h"
#include "LifecycleSystem.h"

#include "Ember/Scene/Scene.h"

namespace Ember {

	void LifecycleSystem::OnUpdate(TimeStep delta, Scene* scene)
	{
		// Update lifecycle entities times and remove any that have expired
		auto& registry = scene->GetRegistry();
		auto view = registry.ActiveQuery<LifetimeComponent>();

		std::vector<EntityID> entitiesToRemove;	// TODO: Find way to optimize vector resizing
		for (auto entity : view)
		{
			auto& comp = registry.GetComponent<LifetimeComponent>(entity);
			comp.Lifetime -= delta.Seconds();

			if (comp.Lifetime <= 0.0f)
			{
				if (registry.ContainsComponent<PoolComponent>(entity))
				{
					auto poolID = registry.GetComponent<PoolComponent>(entity).PoolID;
					scene->GetPoolManager().ReturnToPool(entity, poolID);
				}
				else
				{
					entitiesToRemove.push_back(entity);
				}
			}
		}

		// Remove expired entities after the loop to avoid modifying the registry while iterating
		for (EntityID entity : entitiesToRemove)
			scene->RemoveEntity({ entity, scene });
	}

}