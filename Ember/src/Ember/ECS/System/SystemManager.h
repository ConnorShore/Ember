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

		void RegisterSystem(const SharedPtr<System>& system);
		void UnregisterSystem(const SharedPtr<System>& system);
		void UpdateSystems(TimeStep delta, Scene* scene);

		template<typename T>
		SharedPtr<T> GetSystem()
		{
			for (auto system : m_Systems)
			{
				if (auto castedSystem = DynamicPointerCast<T>(system))
				{
					return castedSystem;
				}
			}
		}

	private:
		bool ContainsSystem(const SharedPtr<System>& system);

	private:
		std::vector<SharedPtr<System>> m_Systems;
	};
}