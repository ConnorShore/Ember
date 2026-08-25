#pragma once

#include "Types.h"
#include "Ember/ECS/Entity/EntityManager.h"
#include "Ember/ECS/Component/ComponentManager.h"
#include "Ember/ECS/System/SystemManager.h"

#include "Ember/Core/Core.h"

#include <algorithm>
#include <concepts>
#include <tuple>
#include <vector>

namespace Ember {

	struct DisabledComponent;

	//template<typename Driver, typename ExcludeType, typename... Filters>
	//class View;

	template<typename Driver, typename FilterTuple, typename ExcludeType>
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

		template<typename T, typename... Args>
		inline T& AttachComponent(EntityID entity, Args&&... args)
		{
			ComponentType type = m_ComponentManager->GetComponentType<T>();
			m_EntityManager->AttachComponent(entity, type);

			T& ret = m_ComponentManager->AttachComponent<T>(entity, std::forward<Args>(args)...);
			if (m_ComponentAttachRegistry.find(type) != m_ComponentAttachRegistry.end())
			{
				auto* attachRegistry = StaticPointerCast<ComponentLifecycleRegistry<T>>(m_ComponentAttachRegistry[type]);
				attachRegistry->Trigger(entity, ret);
			}
			return ret;
		}

		// Copies one component of a runtime-known type from an entity in `sourceRegistry`, firing
		// the same attach hooks the templated AttachComponent does.
		inline bool CopyComponent(Registry& sourceRegistry, EntityID source, EntityID destination, ComponentType type)
		{
			if (!sourceRegistry.m_EntityManager->ContainsComponent(source, type))
				return false;

			if (!m_ComponentManager->CopyComponent(*sourceRegistry.m_ComponentManager, source, destination, type))
			{
				EB_CORE_WARN("Component type {} is not copyable - specialize ComponentCopyTraits<T> for it!", type);
				return false;
			}

			m_EntityManager->AttachComponent(destination, type);

			auto attachRegistry = m_ComponentAttachRegistry.find(type);
			if (attachRegistry != m_ComponentAttachRegistry.end())
				attachRegistry->second->TriggerErased(destination, m_ComponentManager->GetComponentDataErased(destination, type));

			return true;
		}

		inline bool CopyComponent(EntityID source, EntityID destination, ComponentType type)
		{
			return CopyComponent(*this, source, destination, type);
		}

		// Copies every component the source entity has, then mirrors its component ordering so the
		// copy keeps the same inspector layout. Excluded types are left untouched on the destination.
		inline void CopyComponents(Registry& sourceRegistry, EntityID source, EntityID destination, const std::vector<ComponentType>& excluded = {})
		{
			// Copied by value: the attach hooks below can attach components to the source entity.
			const std::vector<ComponentType> order = sourceRegistry.m_EntityManager->GetComponentOrder(source);

			for (ComponentType type : order)
			{
				if (std::find(excluded.begin(), excluded.end(), type) == excluded.end())
					CopyComponent(sourceRegistry, source, destination, type);
			}

			m_EntityManager->SetComponentOrder(destination, order);
		}

		inline void CopyComponents(EntityID source, EntityID destination, const std::vector<ComponentType>& excluded = {})
		{
			CopyComponents(*this, source, destination, excluded);
		}

		template<typename... Excluded>
		inline void CopyComponents(Registry& sourceRegistry, EntityID source, EntityID destination, Exclude<Excluded...>)
		{
			CopyComponents(sourceRegistry, source, destination, std::vector<ComponentType>{ GetComponentType<Excluded>()... });
		}

		template<typename... Excluded>
		inline void CopyComponents(EntityID source, EntityID destination, Exclude<Excluded...> excluded)
		{
			CopyComponents(*this, source, destination, excluded);
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
			// Trigger callbacks if they exist
			if (m_ComponentDetachRegistry.find(type) != m_ComponentDetachRegistry.end())
			{
				// You will need a method in your ComponentManager that returns the raw void* component data given an entity and type ID
				void* rawComponentData = m_ComponentManager->GetComponentDataErased(entity, type);

				m_ComponentDetachRegistry[type]->TriggerErased(entity, rawComponentData);
			}

			// Remove it
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

		inline const std::vector<ComponentType>& GetComponentOrder(EntityID entity) const
		{
			return m_EntityManager->GetComponentOrder(entity);
		}

		inline void SetComponentOrder(EntityID entity, const std::vector<ComponentType>& componentOrder)
		{
			m_EntityManager->SetComponentOrder(entity, componentOrder);
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

		// Standard Query (No Excludes)
		template<typename Driver, typename... Args>
		inline auto Query();

		// Explicit Exclude Query
		// Notice how ExcludeType bypasses the variadic pack deduction errors!
		template<typename Driver, typename... Args, typename ExcludeType>
		inline auto Query(ExcludeType);

		// Gameplay Wrapper (Auto-hides DisabledComponent)
		template<typename Driver, typename... Args>
		inline auto ActiveQuery();

		// Gameplay Wrapper WITH additional custom excludes
		template<typename Driver, typename... Args, typename ExcludeType>
		inline auto ActiveQuery(ExcludeType);

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
	//template<typename Driver, typename... Args>
	//inline View<Driver, Args...> Registry::Query()
	//{
	//	return View<Driver, Args...>(this);
	//}

	template<typename Driver, typename... Args>
	inline auto Registry::Query()
	{
		return View<Driver, std::tuple<Args...>, Exclude<>>(this);
	}

	template<typename Driver, typename... Args, typename ExcludeType>
	inline auto Registry::Query(ExcludeType)
	{
		return View<Driver, std::tuple<Args...>, ExcludeType>(this);
	}

	template<typename Driver, typename... Args>
	inline auto Registry::ActiveQuery()
	{
		return View<Driver, std::tuple<Args...>, Exclude<DisabledComponent>>(this);
	}

	template<typename Driver, typename... Args, typename ExcludeType>
	inline auto Registry::ActiveQuery(ExcludeType)
	{
		// Merges their custom excludes with the DisabledComponent!
		using MergedExcludes = typename MergeExcludes<ExcludeType, Exclude<DisabledComponent>>::Type;

		return View<Driver, std::tuple<Args...>, MergedExcludes>(this);
	}
}