#pragma once

#include "Ember/ECS/Types.h"
#include "Ember/Core/Core.h"
#include "Ember/ECS/Component/Components.h"

#include <string>

namespace Ember {

	class Scene;

	class Entity
	{
	public:
		Entity(const std::string& tag, Scene* scene);
		Entity(EntityID entity, Scene* scene);

		Entity() = default;
		~Entity() = default;

		template<typename T>
		inline T& AttachComponent();

		template<typename T>
		inline void AttachComponent(T& component);

		template<typename T>
		inline void DetachComponent();

		template<typename T>
		inline T& GetComponent();

		inline EntityID GetEntityHandle() const { return m_EntityHandle; }
		const std::string& GetName() const;

		operator EntityID() { return m_EntityHandle; }

	private:
		Scene* m_SceneHandle;
		EntityID m_EntityHandle;
	};

}

// Template implementations included after Scene definition to break circular dependency

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
	inline T& Entity::GetComponent()
	{
		return m_SceneHandle->GetRegistry().GetComponent<T>(m_EntityHandle);
	}

}