#pragma once

#include "System.h"
#include "Ember/Core/Core.h"

#include <vector>

namespace Ember {

	class Registry;

	class SystemManager
	{
	public:
		SystemManager() = default;
		~SystemManager();

		void RegisterSystem(const SharedPtr<System>& system, Registry* registry);
		void UnregisterSystem(const SharedPtr<System>& system, Registry* registry);
		void UpdateSystems(TimeStep delta, Registry* registry);
	private:
		bool ContainsSystem(const SharedPtr<System>& system);

	private:
		std::vector<SharedPtr<System>> m_Systems;
	};
}