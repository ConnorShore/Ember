#pragma once

#include "Ember/Core/Core.h"
#include "Ember/ECS/Registry.h"
#include "Ember/Render/Camera.h"

namespace Ember {

	class Scene : public SharedResource
	{
	public:
		Scene(const std::string name);
		~Scene();

		void OnUpdate(TimeStep delta);

		Entity AddEntity();
		void RemoveEntity(const Entity& entity);

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

	private:
		ScopedPtr<Registry> m_Registry;
		std::string m_Name;
		OrthographicCamera m_Camera;
	};

}