#pragma once

#include "Ember/ECS/Types.h"

#include <queue>
#include <bitset>
#include <array>
#include <vector>

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
		const std::vector<ComponentType>& GetComponentOrder(EntityID entity) const;
		void SetComponentOrder(EntityID entity, const std::vector<ComponentType>& componentOrder);

		inline uint32_t GetNumActiveEntities() const { return m_NextEntityId - static_cast<uint32_t>(m_UnusedIds.size()); }

	private:
		std::bitset<Constants::Entities::MaxEntities> m_AliveEntities;
		std::array<std::bitset<Constants::Entities::MaxComponents>, Constants::Entities::MaxEntities> m_EntityComponentMask;
		std::array<std::vector<ComponentType>, Constants::Entities::MaxEntities> m_EntityComponentOrder;
		std::queue<EntityID> m_UnusedIds;
		uint32_t m_NextEntityId;
	};

}