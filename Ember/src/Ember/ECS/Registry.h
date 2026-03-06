#pragma once

#include "Types.h"
#include "Entity.h"
#include "EntityManager.h"
#include "ComponentManager.h"
#include "SystemManager.h"

#include "Ember/Core/Core.h"

#include <concepts>

namespace Ember {

	class Registry
	{
	public:
		Entity CreateEntity();
		void DestroyEntity(Entity entity);
		void DetatchComponent(Entity entity, ComponentType type);

		template<typename T>
		inline void AttachComponent(Entity entity, T component)
		{

		}

	private:
		ScopedPtr<EntityManager> m_EntityManager;
		ScopedPtr<ComponentManager> m_ComponentManager;
		ScopedPtr<SystemManager> m_SystemManager;
	};

}