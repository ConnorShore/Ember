#pragma once

#include "Ember/Core/Core.h"
#include "Ember/ECS/Registry.h"
#include "Ember/Event/Event.h"
#include "Ember/Event/WindowEvent.h"

#include <unordered_map>

namespace Ember {

	class SceneEntity;

	class Scene : public SharedResource
	{
	public:
		Scene(const std::string& name);
		~Scene();

		void OnUpdate(TimeStep delta);
		void OnEvent(Event& event);

		SceneEntity AddEntity();
		SceneEntity GetEntity(const std::string& tag);
		void RemoveEntity(const SceneEntity& entity);

		template<typename T>
		inline void AttachComponent(const SceneEntity& entity, T& component);

		template<typename T>
		inline void DetachComponent(const SceneEntity& entity);

		template<typename T>
		inline T& GetComponent(const SceneEntity& entity);

		Registry& GetRegistry() { return *m_Registry; }

	private:
		bool OnWindowResize(const WindowResizeEvent& event);

	private:
		ScopedPtr<Registry> m_Registry;

		std::unordered_map<std::string, Entity> m_SceneEntities;

		std::string m_Name;
	};

}

#include "SceneEntity.h"

namespace Ember {

	template<typename T>
	inline void Scene::AttachComponent(const SceneEntity& entity, T& component)
	{
		Entity entityHandle = entity.GetEntityHandle();
		m_Registry->AttachComponent<T>(entityHandle, component);
	}

	template<typename T>
	inline void Scene::DetachComponent(const SceneEntity& entity)
	{
		Entity entityHandle = entity.GetEntityHandle();
		m_Registry->DetachComponent<T>(entityHandle);
	}

	template<typename T>
	inline T& Scene::GetComponent(const SceneEntity& entity)
	{
		Entity entityHandle = entity.GetEntityHandle();
		return m_Registry->GetComponent<T>(entityHandle);
	}

}