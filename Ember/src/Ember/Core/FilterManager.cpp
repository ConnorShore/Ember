#include "ebpch.h"
#include "FilterManager.h"

namespace Ember {

	FilterManager::FilterManager()
	{
		// Ensure the array is completely clear on startup
		for (uint32_t i = 0; i < MaxSlots; i++)
			m_Slots[i] = "";
	}

	void FilterManager::InitWithFilters(const std::array<std::string, MaxSlots>& filters /*= {}*/)
	{
		for (uint32_t i = 0; i < MaxSlots; i++)
			m_Slots[i] = filters[i];
	}

	std::string FilterManager::GetFilterNameBySlot(uint32_t index) const
	{
		if (index < MaxSlots)
			return m_Slots[index];
		return "";
	}

	void FilterManager::SetFilterNameAtSlot(uint32_t index, const std::string& name)
	{
		if (index < MaxSlots)
			m_Slots[index] = name;
	}

	Filter FilterManager::GetFilter(const std::string& name) const
	{
		if (name.empty())
			return 0;

		for (uint32_t i = 0; i < MaxSlots; i++)
		{
			if (m_Slots[i] == name)
				return static_cast<Filter>(1 << i); // Return the exact bit for this slot
		}

		return 0; // Not found
	}

	std::string FilterManager::GetFilterName(Filter filter) const
	{
		for (uint32_t i = 0; i < MaxSlots; i++)
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
		for (uint32_t i = 0; i < MaxSlots; i++)
		{
			if (!m_Slots[i].empty())
				names.push_back(m_Slots[i]);
		}
		return names;
	}

	const std::array<std::string, FilterManager::MaxSlots>& FilterManager::GetAllSlots() const
	{
		return m_Slots;
	}

	std::vector<std::string> FilterManager::GetActiveFilters(Filter activeFilter) const
	{
		std::vector<std::string> names;
		for (uint32_t i = 0; i < MaxSlots; i++)
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