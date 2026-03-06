#pragma once

#include "Types.h"

namespace Ember {

	class Entity
	{
	public:
		Entity()
			: m_Id(EB_INVALID_ENTITY_ID) { }
		Entity(EntityID id)
			: m_Id(id) { }
		~Entity() = default;

		operator EntityID() { return m_Id; }
		operator size_t() { return m_Id; }

	private:
		EntityID m_Id;
	};

}