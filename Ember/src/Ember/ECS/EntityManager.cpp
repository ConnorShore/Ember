#include "ebpch.h"
#include "EntityManager.h"
#include "Ember/Core/Core.h"

namespace Ember {

	EntityManager::EntityManager()
		: m_NextEntityId(0)
	{
	}

	Entity EntityManager::CreateEntity()
	{
		if (!m_UnusedIds.empty())
		{
			EntityID id = m_UnusedIds.front();
			m_UnusedIds.pop();
			m_AliveEntities.set(id, true);
			return id;
		}
		
		EB_CORE_ASSERT(m_NextEntityId < EB_MAX_ENTITIES, "Entity count has reached Ember's max limit!");
		return m_NextEntityId++;
	}

	void EntityManager::DestroyEntity(Entity entity)
	{
		EB_CORE_ASSERT(m_AliveEntities.test(entity), "Entity is already destroyed!");

		m_EntityComponentMask[entity].reset();
		m_AliveEntities.set(entity, false);
		m_UnusedIds.emplace(entity);
	}

	void EntityManager::AddComponent(Entity entity, ComponentType component)
	{
		EB_CORE_ASSERT(component < EB_MAX_COMPONENTS, "Entity has already reached it's component count limit!");
		m_EntityComponentMask[entity].set(component, true);
	}

	void EntityManager::RemoveComponent(Entity entity, ComponentType component)
	{
		EB_CORE_ASSERT(component < EB_MAX_COMPONENTS, "Component is out of bounds!");
		m_EntityComponentMask[entity].set(component, false);
	}

}