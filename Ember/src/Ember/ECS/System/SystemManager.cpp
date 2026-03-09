#include "ebpch.h"
#include "SystemManager.h"

#include "Ember/Core/Core.h"

namespace Ember {

	SystemManager::~SystemManager()
	{
		m_Systems.clear();
	}

	void SystemManager::RegisterSystem(const SharedPtr<System>& system, Registry* registry)
	{
		EB_CORE_ASSERT(!ContainsSystem(system), "System already is registered!");
		system->OnAttach(registry);
		m_Systems.push_back(system);
	}

	void SystemManager::UnregisterSystem(const SharedPtr<System>& system, Registry* registry)
	{
		EB_CORE_ASSERT(ContainsSystem(system), "System is not currently registered!");
		system->OnDetach(registry);
		std::erase(m_Systems, system);
	}

	void SystemManager::UpdateSystems(TimeStep delta, Registry* registry)
	{
		for (const auto& s : m_Systems)
			s->OnUpdate(delta, registry);
	}

	bool SystemManager::ContainsSystem(const SharedPtr<System>& system)
	{
		auto it = std::find(m_Systems.begin(), m_Systems.end(), system);
		return it != m_Systems.end();
	}

}