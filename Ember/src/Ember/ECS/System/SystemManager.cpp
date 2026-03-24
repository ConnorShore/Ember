#include "ebpch.h"
#include "SystemManager.h"

#include "Ember/Core/Core.h"

namespace Ember {

	SystemManager::~SystemManager()
	{
		m_Systems.clear();
	}

	void SystemManager::RegisterSystem(const SharedPtr<System>& system)
	{
		EB_CORE_ASSERT(!ContainsSystem(system), "System already is registered!");
		system->OnAttach();
		m_Systems.push_back(system);
	}

	void SystemManager::UnregisterSystem(const SharedPtr<System>& system)
	{
		EB_CORE_ASSERT(ContainsSystem(system), "System is not currently registered!");
		system->OnDetach();
		std::erase(m_Systems, system);
	}

	void SystemManager::UpdateSystems(TimeStep delta, Scene* scene)
	{
		for (const auto& s : m_Systems)
			s->OnUpdate(delta, scene);
	}

	bool SystemManager::ContainsSystem(const SharedPtr<System>& system)
	{
		auto it = std::ranges::find(m_Systems, system);
		return it != m_Systems.end();
	}

}