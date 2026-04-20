#pragma once

#include <cstdint>
#include <format>
#include <functional>

namespace Ember {

	class UUID
	{
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID&) = default;

		operator uint64_t() const { return m_UUID; }
		bool operator==(const UUID& other) const { return m_UUID == other.m_UUID; }
		bool operator!=(const UUID& other) const { return m_UUID != other.m_UUID; }
		bool operator==(uint64_t other) const { return m_UUID == other; }
		bool operator!=(uint64_t other) const { return m_UUID != other; }

		// Used for sorting
		bool operator<(const UUID& other) const { return m_UUID < other.m_UUID; }

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

template<>
struct std::formatter<Ember::UUID> : std::formatter<uint64_t> {
	auto format(const Ember::UUID& uuid, std::format_context& ctx) const {
		return std::formatter<uint64_t>::format(static_cast<uint64_t>(uuid), ctx);
	}
};