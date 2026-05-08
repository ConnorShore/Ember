#include "ebpch.h"
#include "FilterManager.h"

namespace Ember {

	FilterManager::FilterManager()
	{
		// Ensure the array is completely clear on startup
		for (int i = 0; i < 16; i++)
			m_Slots[i] = "";
	}

	void FilterManager::InitWithFilters(const std::vector<std::string>& filters /*= {}*/)
	{
		uint32_t slotIndex = 0;
		for (const auto& filterName : filters)
		{
			if (slotIndex >= 16)
			{
				EB_CORE_WARN("Exceeded maximum number of filters (16). Skipping: {}", filterName);
				break;
			}
			m_Slots[slotIndex] = filterName;
			slotIndex++;
		}
	}

	std::string FilterManager::GetFilterNameBySlot(uint32_t index) const
	{
		if (index < 16)
			return m_Slots[index];
		return "";
	}

	void FilterManager::SetFilterNameAtSlot(uint32_t index, const std::string& name)
	{
		if (index < 16)
			m_Slots[index] = name;
	}

	Filter FilterManager::GetFilter(const std::string& name) const
	{
		if (name.empty())
			return 0;

		for (int i = 0; i < 16; i++)
		{
			if (m_Slots[i] == name)
				return static_cast<Filter>(1 << i); // Return the exact bit for this slot
		}

		return 0; // Not found
	}

	std::string FilterManager::GetFilterName(Filter filter) const
	{
		for (int i = 0; i < 16; i++)
		{
			if (filter == static_cast<Filter>(1 << i))
				return m_Slots[i];
		}

		// If multiple bits are set, or it's empty
		return "Multiple";
	}

	std::vector<std::string> FilterManager::GetFilters() const
	{
		std::vector<std::string> names;
		for (int i = 0; i < 16; i++)
		{
			if (!m_Slots[i].empty())
				names.push_back(m_Slots[i]);
		}
		return names;
	}

	std::vector<std::string> FilterManager::GetActiveFilters(Filter activeFilter) const
	{
		std::vector<std::string> names;
		for (int i = 0; i < 16; i++)
		{
			if (m_Slots[i].empty())
				continue;

			// Extract the bit for the current slot and check if it's active in the mask
			Filter bit = static_cast<Filter>(1 << i);
			if ((activeFilter & bit) == bit)
			{
				names.push_back(m_Slots[i]);
			}
		}
		return names;
	}

}