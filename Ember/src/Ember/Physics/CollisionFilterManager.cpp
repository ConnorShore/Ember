#include "ebpch.h"
#include "CollisionFilterManager.h"

namespace Ember {

	void CollisionFilterManager::InitWithCustomFilters(std::vector<std::string> customFilters)
	{
		InitDefaultFilters();
		for (const auto& filterName : customFilters)
			AddFilter(filterName);
	}

	void CollisionFilterManager::InitDefaultFilters()
	{
		m_filterMap["Default"] = CollisionFilterPreset::Default;	// Default doesn't use a bit, so it doesn't get counted towards the next filter index

		m_filterMap["Player"] = CollisionFilterPreset::Player;
		m_filterMap["Enemy"] = CollisionFilterPreset::Enemy;
		m_filterMap["Environment"] = CollisionFilterPreset::Environment;

		m_nextFilter = 3; // 3 filters already used by presets, so start from the next
		m_defaultFilterCount = m_filterMap.size();
	}

	void CollisionFilterManager::AddFilter(const std::string& name)
	{
		EB_CORE_ASSERT(m_nextFilter < 64, "Exceeded maximum number of collision filters (64)");

		m_filterMap[name] = 1ULL << m_nextFilter;
		m_nextFilter++;
	}

	CollisionFilter CollisionFilterManager::GetFilter(const std::string& name) const
	{
		if (m_filterMap.find(name) != m_filterMap.end())
			return m_filterMap.at(name);

		return 0ULL;
	}

	std::string CollisionFilterManager::GetFilterName(CollisionFilter filter) const
	{
		for (const auto& [name, f] : m_filterMap)
		{
			if (f == filter)
				return name;
		}

		EB_CORE_ASSERT(false, "Filter not found for value: {}", filter);
		return "Default";
	}

	std::vector<std::string> CollisionFilterManager::GetAllFilters(CollisionFilter filterMask) const
	{
		std::vector<std::string> names;
		names.reserve(m_filterMap.size());
		for (const auto& [name, filter] : m_filterMap)
		{

			// Default filter doesn't count, only set if no filters are active
			if (filter == CollisionFilterPreset::Default)
				continue;
			
			if ((filterMask & filter) == filter)
				names.push_back(name);
		}

		if (names.size() == 0)
			names.push_back(GetFilterName(CollisionFilterPreset::Default));

		return names;
	}

}