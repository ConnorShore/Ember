#pragma once

#include "Types.h"

namespace Ember {

	class Entity
	{
	public:
		Entity(unsigned int id)
			: m_Id(id) { }
		~Entity() = default;

		operator unsigned int() { return m_Id; }

	private:
		EntityID m_Id;
	};

}