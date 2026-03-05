#pragma once

#include "Entity.h"
#include "Types.h"

#include <queue>
#include <bitset>
#include <array>

namespace Ember {

	class EntityManager
	{
	public:
		EntityManager();
		~EntityManager() = default;

		Entity CreateEntity();
		void DestroyEntity(Entity entity);

		void AddComponent(Entity entity, ComponentType component);
		void RemoveComponent(Entity entity, ComponentType component);

		inline const unsigned int GetNumActiveEntities() const { return m_NextEntityId - m_UnusedIds.size(); }

	private:
		std::bitset<EB_MAX_ENTITIES> m_AliveEntities;
		std::array<std::bitset<EB_MAX_COMPONENTS>, EB_MAX_ENTITIES> m_EntityComponentMask;
		std::queue<EntityID> m_UnusedIds;
		unsigned int m_NextEntityId;
	};

}