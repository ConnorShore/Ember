#pragma once

#include <cstdint>
#include <limits>

namespace Ember {

	using EntityID = uint32_t;
	using ComponentType = uint32_t;

	template<typename... Components>
	struct Exclude {};

	template<typename T1, typename T2>
	struct MergeExcludes;

	template<typename... E1, typename... E2>
	struct MergeExcludes<Exclude<E1...>, Exclude<E2...>>
	{
		using Type = Exclude<E1..., E2...>;
	};

}
