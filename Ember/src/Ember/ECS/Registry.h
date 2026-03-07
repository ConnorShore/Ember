#pragma once

#include "Types.h"
#include "Entity.h"
#include "EntityManager.h"
#include "ComponentManager.h"
#include "SystemManager.h"

#include "Ember/Core/Core.h"

#include <concepts>

namespace Ember {

	class Registry
	{
	public:
		Registry();
		~Registry() = default;

		Entity CreateEntity();
		void DestroyEntity(Entity entity);

		template<typename T>
		inline void AttachComponent(Entity entity, T component)
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

		template<typename T>
		inline T& GetComponent(Entity entity)
		{
			return m_ComponentManager->GetComponent<T>(entity);
		}

	private:
		ScopedPtr<EntityManager> m_EntityManager;
		ScopedPtr<ComponentManager> m_ComponentManager;
		ScopedPtr<SystemManager> m_SystemManager;
	};

}