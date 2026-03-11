#include "ebpch.h"
#include "Registry.h"

namespace Ember {

	Registry::Registry()
		: m_EntityManager(ScopedPtr<EntityManager>::Create()),
		m_ComponentManager(ScopedPtr<ComponentManager>::Create()),
		m_SystemManager(ScopedPtr<SystemManager>::Create())
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

	void Registry::RegisterSystem(const SharedPtr<System>& system)
	{
		m_SystemManager->RegisterSystem(system, this);
	}

	void Registry::UnregisterSystem(const SharedPtr<System>& system)
	{
		m_SystemManager->UnregisterSystem(system, this);
	}

	void Registry::UpdateSystems(TimeStep delta)
	{
		m_SystemManager->UpdateSystems(delta, this);
	}

}