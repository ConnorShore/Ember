#pragma once

#include "Ember/ECS/Types.h"

#include <format>

namespace Ember {

	class Entity
	{
	public:
		Entity()
			: m_Id(EB_INVALID_ENTITY_ID) { }
		Entity(EntityID id)
			: m_Id(id) { }
		~Entity() = default;

		operator EntityID() const { return m_Id; }

	private:
		EntityID m_Id;
	};

}

template <>
struct std::formatter<Ember::Entity> : std::formatter<Ember::EntityID>
{
	auto format(const Ember::Entity& entity, format_context& ctx) const
	{
		return std::formatter<Ember::EntityID>::format(static_cast<Ember::EntityID>(entity), ctx);
	}
};