#pragma once

#include "Ember/ECS/Types.h"
#include "Ember/Core/Core.h"
#include "Ember/ECS/Registry.h"
#include "Ember/Event/Event.h"
#include "Ember/Event/WindowEvent.h"


#include <unordered_map>

namespace Ember {

	class Entity;

	class Scene : public SharedResource
	{
	public:
		Scene(const std::string& name);
		~Scene();

		void OnUpdate(TimeStep delta);
		void OnEvent(Event& event);

		Entity AddEntity();
		Entity GetEntity(const std::string& tag);
		void RemoveEntity(const Entity& entity);

		template<typename T>
		inline void AttachComponent(const Entity& entity, T& component);

		template<typename T>
		inline void DetachComponent(const Entity& entity);

		template<typename T>
		inline T& GetComponent(const Entity& entity);

		Registry& GetRegistry() { return *m_Registry; }

	private:
		bool OnWindowResize(const WindowResizeEvent& event);

	private:
		ScopedPtr<Registry> m_Registry;

		std::unordered_map<std::string, EntityID> m_SceneEntities;

		std::string m_Name;
	};

}

#include "Entity.h"

namespace Ember {

	template<typename T>
	inline void Scene::AttachComponent(const Entity& entity, T& component)
	{
		EntityID entityHandle = entity.GetEntityHandle();
		m_Registry->AttachComponent<T>(entityHandle, component);
	}

	template<typename T>
	inline void Scene::DetachComponent(const Entity& entity)
	{
		EntityID entityHandle = entity.GetEntityHandle();
		m_Registry->DetachComponent<T>(entityHandle);
	}

	template<typename T>
	inline T& Scene::GetComponent(const Entity& entity)
	{
		EntityID entityHandle = entity.GetEntityHandle();
		return m_Registry->GetComponent<T>(entityHandle);
	}

}