#pragma once

#include "CollisionFilter.h"

#include <string>
#include <unordered_map>

namespace Ember {

	class CollisionFilterManager
	{
	public:
		CollisionFilterManager() = default;

		void InitWithCustomFilters(std::vector<std::string> customFilters);
		void InitDefaultFilters();
		void AddFilter(const std::string& name);

		CollisionFilter GetFilter(const std::string& name) const;
		std::string GetFilterName(CollisionFilter filter) const;
		std::vector<std::string> GetAllFilters(CollisionFilter filterMask) const;

		inline std::vector<std::string> GetFilters() const
		{
			std::vector<std::string> names;
			names.reserve(m_filterMap.size());
			for (const auto& [name, filter] : m_filterMap)
				names.push_back(name);
			return names;
		}
		inline std::vector<std::string> GetCustomFilters() const		{
			std::vector<std::string> names;
			names.reserve(m_filterMap.size() - m_defaultFilterCount);
			for (const auto& [name, filter] : m_filterMap)
			{
				if (filter != CollisionFilterPreset::Default &&
					filter != CollisionFilterPreset::Player &&
					filter != CollisionFilterPreset::Enemy &&
					filter != CollisionFilterPreset::Environment)
				{
					names.push_back(name);
				}
			}
			return names;
		}

		inline uint32_t GetFilterCount() const { return static_cast<uint32_t>(m_filterMap.size()); }
		inline uint32_t GetCustomFilterCount() const { return static_cast<uint32_t>(m_filterMap.size() - m_defaultFilterCount); }

		// Returns names of all filters whose bit is set in activeFilter
		inline std::vector<std::string> GetActiveFilters(CollisionFilter activeFilter) const
		{
			std::vector<std::string> names;
			for (const auto& [name, filter] : m_filterMap)
			{
				if (filter != CollisionFilterPreset::Default && (activeFilter & filter) == filter)
					names.push_back(name);
			}
			return names;
		}

	private:
		std::unordered_map<std::string, CollisionFilter> m_filterMap;
		uint64_t m_nextFilter = 0;
		uint64_t m_defaultFilterCount = 0;
	};

}