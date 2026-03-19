#pragma once

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

		EntityID CreateEntity();
		void DestroyEntity(EntityID entity);

		void AttachComponent(EntityID entity, ComponentType component);
		void DetachComponent(EntityID entity, ComponentType component);

		bool ContainsComponent(EntityID entity, ComponentType component);

		inline const unsigned int GetNumActiveEntities() const { return m_NextEntityId - m_UnusedIds.size(); }

	private:
		std::bitset<Constants::Entities::MaxEntities> m_AliveEntities;
		std::array<std::bitset<Constants::Entities::MaxComponents>, Constants::Entities::MaxEntities> m_EntityComponentMask;
		std::queue<EntityID> m_UnusedIds;
		unsigned int m_NextEntityId;
	};

}