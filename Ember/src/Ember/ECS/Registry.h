#pragma once

#include "Types.h"
#include "Ember/ECS/Entity/Entity.h"
#include "Ember/ECS/Entity/EntityManager.h"
#include "Ember/ECS/Component/ComponentManager.h"
#include "Ember/ECS/System/SystemManager.h"

#include "Ember/Core/Core.h"

#include <concepts>
#include <tuple>

namespace Ember {

	template<typename Driver, typename... Filters>
	class View;

	class Registry
	{
	public:
		Registry();
		~Registry() = default;

		Entity CreateEntity();
		void DestroyEntity(Entity entity);

		template<typename T>
		inline void AttachComponent(Entity entity, T& component)
		{
			ComponentType type = m_ComponentManager->GetComponentType<T>();

			m_EntityManager->AttachComponent(entity, type);
			m_ComponentManager->AttachComponent<T>(entity, component);
		}

		template<typename T>
		inline void DetachComponent(Entity entity)
		{
			ComponentType type = m_ComponentManager->GetComponentType<T>();

			m_EntityManager->DetachComponent(entity, type);
			m_ComponentManager->DetachComponent<T>(entity);
		}

		template<typename T>
		inline bool ContainsComponent(Entity entity)
		{
			ComponentType type = m_ComponentManager->GetComponentType<T>();
			return m_EntityManager->ContainsComponent(entity, type);
		}

		template<typename... Args>
		inline bool ContainsComponents(Entity entity)
		{
			return (ContainsComponent<Args>(entity) && ...);
		}

		template<typename T>
		inline T& GetComponent(Entity entity)
		{
			return m_ComponentManager->GetComponent<T>(entity);
		}

		template<typename... Args>
		inline std::tuple<Args&...> GetComponents(Entity entity)
		{
			return std::forward_as_tuple(GetComponent<Args>(entity)...);
		}

		template<typename Driver, typename... Args>
		inline View<Driver, Args...> Query();

		template<typename T>
		const std::vector<EntityID>& GetActiveEntities()
		{
			return m_ComponentManager->GetActiveEntities<T>();
		}

		void RegisterSystem(const SharedPtr<System>& system);
		void UnregisterSystem(const SharedPtr<System>& system);
		void UpdateSystems(TimeStep delta);

	private:
		ScopedPtr<EntityManager> m_EntityManager;
		ScopedPtr<ComponentManager> m_ComponentManager;
		ScopedPtr<SystemManager> m_SystemManager;
	};
}

// Including the implementation of the template function here since it's header-only
// And also to avoid circular dependency issues since View also needs to reference Registry

#include "View.h"

namespace Ember {
	template<typename Driver, typename... Args>
	inline View<Driver, Args...> Registry::Query()
	{
		return View<Driver, Args...>(this);
	}
}