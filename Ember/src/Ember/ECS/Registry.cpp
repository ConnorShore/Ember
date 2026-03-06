#include "ebpch.h"
#include "Registry.h"

namespace Ember {

	Registry::Registry()
		: m_EntityManager(ScopedPtr<EntityManager>::Create()),
		m_ComponentManager(ScopedPtr<ComponentManager>::Create()),
		m_SystemManager(nullptr)
	{
	}

	Entity Registry::CreateEntity()
	{
		return m_EntityManager->CreateEntity();
	}

	void Registry::DestroyEntity(Entity entity)
	{
		m_EntityManager->DestroyEntity(entity);
		m_ComponentManager->EntityDestroyed(entity);
	}

}