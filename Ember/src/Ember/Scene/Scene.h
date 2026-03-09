#pragma once

#include "Ember/Core/Core.h"
#include "Ember/ECS/Registry.h"
#include "Ember/Render/Camera.h"

#include <unordered_map>

namespace Ember {

	class SceneEntity;

	class Scene : public SharedResource
	{
	public:
		Scene(const std::string& name);
		~Scene();

		void OnUpdate(TimeStep delta);

		SharedPtr<SceneEntity> AddEntity();
		SharedPtr<SceneEntity> GetEntity(const std::string& tag);
		void RemoveEntity(const SharedPtr<SceneEntity>& entity);

		template<typename T>
		void AttachComponent(const Entity& entity, T& component)
		{
			m_Registry->AttachComponent<T>(entity, component);
		}

		template<typename T>
		void DetachComponent(const Entity& entity)
		{
			m_Registry->DetachComponent<T>(entity);
		}

		template<typename T>
		T& GetComponent(const Entity& entity)
		{
			return m_Registry->GetComponent<T>(entity);
		}

		Registry& GetRegistry() { return *m_Registry; }

	private:
		ScopedPtr<Registry> m_Registry;

		std::unordered_map<std::string, SharedPtr<SceneEntity>> m_SceneEntities;

		std::string m_Name;
		OrthographicCamera m_Camera;
	};

}

#include "SceneEntity.h"
