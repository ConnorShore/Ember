#include "ebpch.h"
#include "Registry.h"

namespace Ember {

	Registry::Registry()
		: m_EntityManager(ScopedPtr<EntityManager>::Create()),
		m_ComponentManager(ScopedPtr<ComponentManager>::Create())
	{
	}

	EntityID Registry::CreateEntity()
	{
		return m_EntityManager->CreateEntity();
	}

	void Registry::DestroyEntity(EntityID entity)
	{
		m_ComponentManager->EntityDestroyed(entity);
		m_EntityManager->DestroyEntity(entity);
	}

}