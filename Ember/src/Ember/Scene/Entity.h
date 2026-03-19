#pragma once

#include "Ember/ECS/Types.h"
#include "Ember/Core/Core.h"
#include "Ember/ECS/Component/Components.h"

#include <string>
#include <vector>

namespace Ember {

	class Scene;

	class Entity
	{
	public:
		Entity(const std::string& tag, Scene* scene);
		Entity(EntityID entity, Scene* scene);

		Entity() : m_EntityHandle(Constants::Entities::InvalidEntityID), m_SceneHandle(nullptr) {}
		~Entity() = default;

		template<typename T>
		inline T& AttachComponent();

		template<typename T>
		inline void AttachComponent(T& component);

		template<typename T>
		inline void DetachComponent();

		template<typename T>
		inline bool ContainsComponent();

		template<typename T>
		inline T& GetComponent();

		std::vector<Entity> GetAllChildren();

		Entity GetChildByName(const std::string& name);
		Entity FindEntityInHierarchy(const std::string& name);

		inline EntityID GetEntityHandle() const { return m_EntityHandle; }
		const std::string& GetName() const;

		operator EntityID() { return m_EntityHandle; }

	private:
		Scene* m_SceneHandle;
		EntityID m_EntityHandle;
	};

}

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

	template<typename T>
	inline bool Entity::ContainsComponent()
	{
		return m_SceneHandle->GetRegistry().ContainsComponent<T>(m_EntityHandle);
	}

	template<typename T>
	inline T& Entity::GetComponent()
	{
		return m_SceneHandle->GetRegistry().GetComponent<T>(m_EntityHandle);
	}

}