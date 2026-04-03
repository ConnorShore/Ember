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

		template<typename T>
		inline T& AttachComponent();

		template<typename T>
		inline void AttachComponent(T& component);

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

		std::vector<Entity> GetAllChildren();
		uint32_t GetNumChildren();
		bool IsRootParent();

		Entity GetChildByName(const std::string& name);
		Entity FindEntityInHierarchy(const std::string& name);

		inline EntityID GetEntityHandle() const { return m_EntityHandle; }
		UUID GetUUID() const;
		const std::string& GetName() const;

		operator EntityID() { return m_EntityHandle; }
		const bool operator==(const Entity& other) const { return m_EntityHandle == other.m_EntityHandle && m_SceneHandle == other.m_SceneHandle; }// Returns true if the entity has a valid handle and points to a valid scene
		operator bool() const { return m_EntityHandle != Constants::Entities::InvalidEntityID && m_SceneHandle != nullptr; }

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

	template<typename T>
	inline void Entity::AttachComponent(T& component)
	{
		m_SceneHandle->GetRegistry().AttachComponent<T>(m_EntityHandle, component);
	}

	template<typename T>
	T& Entity::AttachComponent()
	{
		T component;
		return m_SceneHandle->GetRegistry().AttachComponent<T>(m_EntityHandle, component);
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

}