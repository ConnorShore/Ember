#pragma once

#include <cstdint>
#include <limits>
#include <type_traits>

namespace Ember {

	using EntityID = uint32_t;
	using ComponentType = uint32_t;

	// Customization point for duplicating a component onto another entity. Specialize it for
	// components that own per-entity runtime state which must not be shared with the copy.
	template<typename T>
	struct ComponentCopyTraits
	{
		static constexpr bool Copyable = std::is_copy_constructible_v<T>;

		static T Copy(const T& source) { return T(source); }
	};

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
