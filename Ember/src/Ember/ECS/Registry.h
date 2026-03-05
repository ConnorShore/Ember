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

		template<std::same_as<ComponentType> T>
		void AttachComponent(Entity entity, T component);
		void DetatchComponent(Entity entity, ComponentType type);

	private:
		ScopedPtr<EntityManager> m_EntityManager;
		ScopedPtr<ComponentManager> m_ComponentManager;
		ScopedPtr<SystemManager> m_SystemManager;
	};
}