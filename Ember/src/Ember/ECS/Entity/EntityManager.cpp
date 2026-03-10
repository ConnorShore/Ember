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
		
		EB_CORE_ASSERT(m_NextEntityId < MaxEntities, "Entity count has reached Ember's max limit!");

		unsigned int newId = m_NextEntityId++;
		m_AliveEntities.set(newId, true);
		return newId;
	}

	void EntityManager::DestroyEntity(Entity entity)
	{
		EB_CORE_ASSERT(m_AliveEntities.test(entity), "Entity is already destroyed!");

		m_EntityComponentMask[entity].reset();
		m_AliveEntities.set(entity, false);
		m_UnusedIds.emplace(entity);
	}

	void EntityManager::AttachComponent(Entity entity, ComponentType component)
	{
		EB_CORE_ASSERT(component < MaxComponents, "Entity has already reached it's component count limit!");
		m_EntityComponentMask[entity].set(component, true);
	}

	void EntityManager::DetachComponent(Entity entity, ComponentType component)
	{
		EB_CORE_ASSERT(component < MaxComponents, "Component is out of bounds!");
		m_EntityComponentMask[entity].set(component, false);
	}

	bool EntityManager::ContainsComponent(Entity entity, ComponentType component)
	{
		EB_CORE_ASSERT(component < MaxComponents, "Component is out of bounds!");
		return m_EntityComponentMask[entity].test(component);
	}

}