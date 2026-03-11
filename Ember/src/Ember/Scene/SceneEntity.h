#pragma once

#include "Ember/Core/Core.h"
#include "Ember/ECS/Entity/Entity.h"
#include "Ember/ECS/Component/Components.h"

#include <string>

namespace Ember {

	class Scene;

	class SceneEntity
	{
	public:
		SceneEntity(const std::string& tag, Scene* scene);
		SceneEntity(Entity entity, Scene* scene);

		SceneEntity() = default;
		~SceneEntity() = default;

		template<typename T>
		inline T& AttachComponent();

		template<typename T>
		inline void AttachComponent(T& component);

		template<typename T>
		inline void DetachComponent();

		template<typename T>
		inline T& GetComponent();

		inline Entity GetEntityHandle() const { return m_EntityHandle; }
		const std::string& GetName() const;

		operator Entity() { return m_EntityHandle; }

	private:
		Scene* m_SceneHandle;
		Entity m_EntityHandle;
	};

}

// Template implementations included after Scene definition to break circular dependency

#include "Scene.h"

namespace Ember {

	template<typename T>
	inline void SceneEntity::AttachComponent(T& component)
	{
		m_SceneHandle->GetRegistry().AttachComponent<T>(m_EntityHandle, component);
	}

	template<typename T>
	T& SceneEntity::AttachComponent()
	{
		T component;
		return m_SceneHandle->GetRegistry().AttachComponent<T>(m_EntityHandle, component);
	}

	template<typename T>
	inline void SceneEntity::DetachComponent()
	{
		m_SceneHandle->GetRegistry().DetachComponent<T>(m_EntityHandle);
	}

	template<typename T>
	inline T& SceneEntity::GetComponent()
	{
		return m_SceneHandle->GetRegistry().GetComponent<T>(m_EntityHandle);
	}

}