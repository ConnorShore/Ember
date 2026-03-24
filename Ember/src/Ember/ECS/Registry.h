#pragma once

#include "Types.h"
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

		EntityID CreateEntity();
		void DestroyEntity(EntityID entity);

		template<typename T>
		inline T& AttachComponent(EntityID entity, T& component)
		{
			ComponentType type = m_ComponentManager->GetComponentType<T>();

			m_EntityManager->AttachComponent(entity, type);
			return m_ComponentManager->AttachComponent<T>(entity, component);
		}

		template<typename T>
		inline void DetachComponent(EntityID entity)
		{
			ComponentType type = m_ComponentManager->GetComponentType<T>();

			m_EntityManager->DetachComponent(entity, type);
			m_ComponentManager->DetachComponent<T>(entity);
		}

		template<typename T>
		inline bool ContainsComponent(EntityID entity)
		{
			ComponentType type = m_ComponentManager->GetComponentType<T>();
			return m_EntityManager->ContainsComponent(entity, type);
		}

		inline bool ContainsComponent(EntityID entity, ComponentType type)
		{
			return m_EntityManager->ContainsComponent(entity, type);
		}

		template<typename... Args>
		inline bool ContainsComponents(EntityID entity)
		{
			return (ContainsComponent<Args>(entity) && ...);
		}

		template<typename T>
		inline T& GetComponent(EntityID entity)
		{
			return m_ComponentManager->GetComponent<T>(entity);
		}

		template<typename... Args>
		inline std::tuple<Args&...> GetComponents(EntityID entity)
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

		template<typename T>
		SharedPtr<T> GetSystem()
		{
			return m_SystemManager->GetSystem<T>();
		}

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