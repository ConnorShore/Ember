#pragma once

#include "Entity.h"
#include "Ember/ECS/Types.h"

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

		void AttachComponent(Entity entity, ComponentType component);
		void DetachComponent(Entity entity, ComponentType component);

		bool ContainsComponent(Entity entity, ComponentType component);

		inline const unsigned int GetNumActiveEntities() const { return m_NextEntityId - m_UnusedIds.size(); }

	private:
		std::bitset<MaxEntities> m_AliveEntities;
		std::array<std::bitset<MaxComponents>, MaxEntities> m_EntityComponentMask;
		std::queue<EntityID> m_UnusedIds;
		unsigned int m_NextEntityId;
	};

}