#pragma once

#include "Ember/ECS/Types.h"
#include "Ember/Core/Core.h"
#include "Ember/ECS/Component/Components.h"

#include <string>
#include <vector>
#include <functional>

namespace Ember {

	class Scene;

	class Entity
	{
	public:
		Entity(EntityID entityHandle, Scene* scene)
			: m_EntityHandle(entityHandle), m_SceneHandle(scene) {}
		Entity()
			: m_EntityHandle(Constants::Entities::InvalidEntityID), m_SceneHandle(nullptr) {}
		Entity(const Entity&) = default;

		~Entity() = default;

		template<typename T, typename... Args>
		inline T& AttachComponent(Args&&... args);

		template<typename T>
		inline void DetachComponent();
		inline void DetachComponent(ComponentType componentType) const;

		template<typename T>
		inline bool ContainsComponent();
		inline bool ContainsComponent(ComponentType componentType) const;

		template<typename T>
		inline T& GetComponent();

		template<typename T>
		inline ComponentType GetComponentType();
		inline const std::vector<ComponentType>& GetComponentOrder() const;
		inline void SetComponentOrder(const std::vector<ComponentType>& componentOrder) const;

		std::vector<Entity> GetAllChildren();
		uint32_t GetNumChildren();
		Entity GetParent();
		Entity GetRootParent();
		bool IsRootParent();

		Entity AddChild(Entity entity);
		Entity AddChild(Entity entity, bool isAttachment);
		Entity AddChild(const std::string& name = "");
		void RemoveFromParent();
		Entity GetChildByName(const std::string& name);
		Entity FindEntityInHierarchy(const std::string& name);

		void SetActive(bool active, bool recursive = true);
		bool IsActive() const;

		inline EntityID GetEntityHandle() const { return m_EntityHandle; }
		UUID GetUUID() const;
		const std::string& GetName() const;
		const bool IsValid() const { return m_EntityHandle != Constants::Entities::InvalidEntityID; }

		// Must stay const: a non-const conversion binds the implicit object argument better than
		// `operator bool() const`, so a boolean context would silently select this one and invert.
		operator EntityID() const { return m_EntityHandle; }
		const bool operator==(const Entity& other) const { return m_EntityHandle == other.m_EntityHandle && m_SceneHandle == other.m_SceneHandle; }
		// Returns true if the entity has a valid handle and points to a valid scene
		explicit operator bool() const { return m_EntityHandle != Constants::Entities::InvalidEntityID && m_SceneHandle != nullptr; }

	private:
		Scene* m_SceneHandle;
		EntityID m_EntityHandle;
	};

}

// Hash function for entity
template<>
struct std::hash<Ember::Entity>
{
	size_t operator()(const Ember::Entity& e) const noexcept
	{
		return std::hash<Ember::EntityID>()(e.GetEntityHandle());
	}
};

#include "Scene.h"

namespace Ember {

	template<typename T, typename... Args>
	inline T& Entity::AttachComponent(Args&&... args)
	{
		return m_SceneHandle->GetRegistry().AttachComponent<T>(m_EntityHandle, std::forward<Args>(args)...);
	}

	template<typename T>
	inline void Entity::DetachComponent()
	{
		m_SceneHandle->GetRegistry().DetachComponent<T>(m_EntityHandle);
	}

	inline void Entity::DetachComponent(ComponentType componentType) const
	{
		EB_CORE_ASSERT(componentType != Constants::Entities::InvalidComponentID, "Invalid component type provided for detachment!");
		EB_CORE_ASSERT(ContainsComponent(componentType), "Entity does not contain component type {}!", componentType);
		m_SceneHandle->GetRegistry().DetachComponent(m_EntityHandle, componentType);
	}

	template<typename T>
	inline bool Entity::ContainsComponent()
	{
		return m_SceneHandle->GetRegistry().ContainsComponent<T>(m_EntityHandle);
	}

	inline bool Entity::ContainsComponent(ComponentType componentType) const
	{
		EB_CORE_ASSERT(componentType != Constants::Entities::InvalidComponentID, "Invalid component type provided for containment check!");
		return m_SceneHandle->GetRegistry().ContainsComponent(m_EntityHandle, componentType);
	}

	template<typename T>
	inline T& Entity::GetComponent()
	{
		return m_SceneHandle->GetRegistry().GetComponent<T>(m_EntityHandle);
	}

	template<typename T>
	inline ComponentType Entity::GetComponentType()
	{
		return m_SceneHandle->GetRegistry().GetComponentType<T>();
	}

	inline const std::vector<ComponentType>& Entity::GetComponentOrder() const
	{
		return m_SceneHandle->GetRegistry().GetComponentOrder(m_EntityHandle);
	}

	inline void Entity::SetComponentOrder(const std::vector<ComponentType>& componentOrder) const
	{
		m_SceneHandle->GetRegistry().SetComponentOrder(m_EntityHandle, componentOrder);
	}

}