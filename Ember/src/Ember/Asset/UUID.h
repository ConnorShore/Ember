#pragma once

#include <cstdint>
#include <functional>

namespace Ember {

	class UUID
	{
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID&) = default;

		operator uint64_t() const { return m_UUID; }
	private:
		uint64_t m_UUID;
	};

}

// Add a custom hash function to the standard library so unordered_map accepts our UUID
namespace std {
	template <typename T> struct hash;

	template<>
	struct hash<Ember::UUID>
	{
		std::size_t operator()(const Ember::UUID& uuid) const
		{
			return (uint64_t)uuid;
		}
	};
}