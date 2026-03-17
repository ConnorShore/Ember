#pragma once

#include <limits>

namespace Ember {

	using EntityID = unsigned int;
	using ComponentType = unsigned int;

	constexpr unsigned int MaxEntities = 1024;
	constexpr unsigned int MaxComponents = 64;

	constexpr unsigned int InvalidEntityID = MaxEntities + 1;
	constexpr unsigned int InvalidComponentID = MaxEntities + 1;

}
