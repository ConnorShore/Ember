#include "ebpch.h"
#include "EntityManager.h"
#include "Ember/Core/Core.h"

namespace Ember {

	EntityManager::EntityManager()
		: m_NextEntityId(0)
	{
	}

	EntityID EntityManager::CreateEntity()
	{
		if (!m_UnusedIds.empty())
		{
			EntityID id = m_UnusedIds.front();
			m_UnusedIds.pop();
			m_AliveEntities.set(id, true);
			return id;
		}
		
		EB_CORE_ASSERT(m_NextEntityId < Constants::Entities::MaxEntities, "Entity count has reached Ember's max limit!");

		uint32_t newId = m_NextEntityId++;
		m_AliveEntities.set(newId, true);
		return newId;
	}

	void EntityManager::DestroyEntity(EntityID entity)
	{
		EB_CORE_ASSERT(m_AliveEntities.test(entity), "Entity is already destroyed!");

		m_EntityComponentMask[entity].reset();
		m_EntityComponentOrder[entity].clear();
		m_AliveEntities.set(entity, false);
		m_UnusedIds.emplace(entity);
	}

	void EntityManager::AttachComponent(EntityID entity, ComponentType component)
	{
		EB_CORE_ASSERT(component < Constants::Entities::MaxComponents, "Entity has already reached it's component count limit!");
		if (!m_EntityComponentMask[entity].test(component))
			m_EntityComponentOrder[entity].push_back(component);

		m_EntityComponentMask[entity].set(component, true);
	}

	void EntityManager::DetachComponent(EntityID entity, ComponentType component)
	{
		EB_CORE_ASSERT(component < Constants::Entities::MaxComponents, "Component is out of bounds!");
		m_EntityComponentMask[entity].set(component, false);

		auto& componentOrder = m_EntityComponentOrder[entity];
		for (auto it = componentOrder.begin(); it != componentOrder.end(); ++it)
		{
			if (*it == component)
			{
				componentOrder.erase(it);
				break;
			}
		}
	}

	bool EntityManager::ContainsComponent(EntityID entity, ComponentType component)
	{
		EB_CORE_ASSERT(component < Constants::Entities::MaxComponents, "Component is out of bounds!");
		return m_EntityComponentMask[entity].test(component);
	}

	const std::vector<ComponentType>& EntityManager::GetComponentOrder(EntityID entity) const
	{
		EB_CORE_ASSERT(m_AliveEntities.test(entity), "Cannot get component order for a destroyed entity!");
		return m_EntityComponentOrder[entity];
	}

	void EntityManager::SetComponentOrder(EntityID entity, const std::vector<ComponentType>& componentOrder)
	{
		EB_CORE_ASSERT(m_AliveEntities.test(entity), "Cannot set component order for a destroyed entity!");

		std::vector<ComponentType> newOrder;
		auto containsComponentType = [&newOrder](ComponentType component) {
			for (ComponentType existingComponent : newOrder)
			{
				if (existingComponent == component)
					return true;
			}

			return false;
			};

		for (ComponentType component : componentOrder)
		{
			if (component >= Constants::Entities::MaxComponents || !m_EntityComponentMask[entity].test(component) || containsComponentType(component))
				continue;

			newOrder.push_back(component);
		}

		for (ComponentType component : m_EntityComponentOrder[entity])
		{
			if (component >= Constants::Entities::MaxComponents || !m_EntityComponentMask[entity].test(component) || containsComponentType(component))
				continue;

			newOrder.push_back(component);
		}

		m_EntityComponentOrder[entity] = std::move(newOrder);
	}

}