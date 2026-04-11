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

	template<typename T>
	using ComponentCallback = std::function<void(EntityID, T&)>;

	class ComponentLifecycleRegistryBase
	{
	public:
		virtual ~ComponentLifecycleRegistryBase() = default;

		virtual void TriggerErased(EntityID entity, void* componentData) = 0;
		virtual void Clear() = 0;
	};

	template<typename T>
	class ComponentLifecycleRegistry : public ComponentLifecycleRegistryBase
	{
	public:

		void Connect(ComponentCallback<T> callback)
		{
			m_ComponentCallbacks.push_back(callback);
		}

		void Trigger(EntityID entity, T& component)
		{
			for (const auto& callback : m_ComponentCallbacks)
			{
				callback(entity, component);
			}
		}

		virtual void TriggerErased(EntityID entity, void* componentData) override
		{
			T& component = *static_cast<T*>(componentData);
			Trigger(entity, component);
		}

		virtual void Clear() override
		{
			m_ComponentCallbacks.clear();
		}

	private:
		std::vector<ComponentCallback<T>> m_ComponentCallbacks;
	};

	class Registry
	{
	public:
		Registry();
		~Registry() = default;

		EntityID CreateEntity();
		void DestroyEntity(EntityID entity);

		template<typename T>
		ComponentLifecycleRegistry<T>& OnComponentAttached()
		{
			ComponentType type = m_ComponentManager->GetComponentType<T>();

			if (m_ComponentAttachRegistry.find(type) == m_ComponentAttachRegistry.end())
				m_ComponentAttachRegistry[type] = ScopedPtr<ComponentLifecycleRegistry<T>>::Create();

			return *StaticPointerCast<ComponentLifecycleRegistry<T>>(m_ComponentAttachRegistry[type]);
		}

		template<typename T>
		ComponentLifecycleRegistry<T>& OnComponentDetached()
		{
			ComponentType type = m_ComponentManager->GetComponentType<T>();

			if (m_ComponentDetachRegistry.find(type) == m_ComponentDetachRegistry.end())
				m_ComponentDetachRegistry[type] = ScopedPtr<ComponentLifecycleRegistry<T>>::Create();

			return *StaticPointerCast<ComponentLifecycleRegistry<T>>(m_ComponentDetachRegistry[type]);
		}

		// Connects a callback for future component attachments, 
		// and immediately fires it for all existing components of type T.
		template<typename T>
		inline void ConnectAndRetroact(ComponentCallback<T> callback)
		{
			// Connect the listener for future runtime additions
			OnComponentAttached<T>().Connect(callback);

			// Backfill: Automatically trigger it for everything currently in the scene!
			const std::vector<EntityID>& existingEntities = GetActiveEntities<T>();
			for (EntityID entity : existingEntities)
			{
				T& component = GetComponent<T>(entity);
				callback(entity, component);
			}
		}

		template<typename T>
		inline T& AttachComponent(EntityID entity, T& component)
		{
			ComponentType type = m_ComponentManager->GetComponentType<T>();

			m_EntityManager->AttachComponent(entity, type);
			T& ret = m_ComponentManager->AttachComponent<T>(entity, component);

			// Trigger the attach callbacks after actually attaching the component
			if (m_ComponentAttachRegistry.find(type) != m_ComponentAttachRegistry.end())
			{
				auto* attachRegistry = StaticPointerCast<ComponentLifecycleRegistry<T>>(m_ComponentAttachRegistry[type]);
				attachRegistry->Trigger(entity, ret);
			}

			return ret;
		}

		template<typename T>
		inline void DetachComponent(EntityID entity)
		{
			ComponentType type = m_ComponentManager->GetComponentType<T>();

			T& component = m_ComponentManager->GetComponent<T>(entity);

			// Trigger the detach callbacks before actually detaching the component
			if (m_ComponentDetachRegistry.find(type) != m_ComponentDetachRegistry.end()) {
				auto* detachRegistry = StaticPointerCast<ComponentLifecycleRegistry<T>>(m_ComponentDetachRegistry[type]);
					detachRegistry->Trigger(entity, component);
			}

			m_EntityManager->DetachComponent(entity, type);
			m_ComponentManager->DetachComponent<T>(entity);
		}

		inline void DetachComponent(EntityID entity, ComponentType type)
		{
			// 1. Trigger callbacks if they exist
			if (m_ComponentDetachRegistry.find(type) != m_ComponentDetachRegistry.end())
			{
				// You will need a method in your ComponentManager that returns the raw void* component data given an entity and type ID
				void* rawComponentData = m_ComponentManager->GetComponentDataErased(entity, type);

				m_ComponentDetachRegistry[type]->TriggerErased(entity, rawComponentData);
			}

			// 2. Remove it
			m_EntityManager->DetachComponent(entity, type);
			m_ComponentManager->DetachComponent(entity, type);
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
		inline ComponentType GetComponentType()
		{
			return m_ComponentManager->GetComponentType<T>();
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

		// Returns a reference to the vector of active entities that have component T
		template<typename T>
		const std::vector<EntityID>& GetActiveEntities()
		{
			return m_ComponentManager->GetActiveEntities<T>();
		}

	private:
		ScopedPtr<EntityManager> m_EntityManager;
		ScopedPtr<ComponentManager> m_ComponentManager;

		std::unordered_map<ComponentType, ScopedPtr<ComponentLifecycleRegistryBase>> m_ComponentAttachRegistry;
		std::unordered_map<ComponentType, ScopedPtr<ComponentLifecycleRegistryBase>> m_ComponentDetachRegistry;
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