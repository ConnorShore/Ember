#pragma once

#pragma once

#include <string>
#include <vector>
#include <array>

namespace Ember {

	template<typename T>
	class FilterManager
	{
	public:
		FilterManager();

		void InitWithFilters(const std::vector<std::string>& filters = {});

		// UI/Editor Array Slot Access
		std::string GetFilterNameBySlot(uint32_t index) const;
		void SetFilterNameAtSlot(uint32_t index, const std::string& name);

		// Runtime Bitmask Access
		T GetFilter(const std::string& name) const;
		std::string GetFilterName(T filter) const;

		// Determine which slots are locked (can't be modified)
		inline bool isSlotLocked(uint16_t i) { return (1 << i) & m_Locked; }
		inline void setSlotLock(uint16_t slot, bool locked) { m_Locked |= ((int)locked) << slot; }

		// Bulk Queries
		std::vector<std::string> GetFilters() const;
		std::vector<std::string> GetActiveFilters(T activeFilter) const;

	private:
		// ReactPhysics3D uses a 16-bit integer mask, giving us exactly 16 fixed slots.
		std::string m_Slots[16];
		uint16_t m_Locked = 0;	// 16 bit integer masked for which slots are locked
	};

	template<typename T>
	FilterManager<T>::FilterManager()
	{
		// Ensure the array is completely clear on startup
		for (int i = 0; i < 16; i++)
			m_Slots[i] = "";
	}

	template<typename T>
	std::vector<std::string> Ember::FilterManager<T>::GetActiveFilters(T activeFilter) const
	{
		std::vector<std::string> names;
		for (int i = 0; i < 16; i++)
		{
			if (m_Slots[i].empty())
				continue;

			// Extract the bit for the current slot and check if it's active in the mask
			T bit = static_cast<T>(1 << i);
			if ((activeFilter & bit) == bit)
			{
				names.push_back(m_Slots[i]);
			}
		}
		return names;
	}

	template<typename T>
	std::vector<std::string> Ember::FilterManager<T>::GetFilters() const
	{
		std::vector<std::string> names;
		for (int i = 0; i < 16; i++)
		{
			if (!m_Slots[i].empty())
				names.push_back(m_Slots[i]);
		}
		return names;
	}

	template<typename T>
	std::string Ember::FilterManager<T>::GetFilterName(T filter) const
	{
		for (int i = 0; i < 16; i++)
		{
			if (filter == static_cast<T>(1 << i))
				return m_Slots[i];
		}

		// If multiple bits are set, or it's empty
		return "Multiple";
	}

	template<typename T>
	T FilterManager<T>::GetFilter(const std::string& name) const
	{
		if (name.empty())
			return 0;

		for (int i = 0; i < 16; i++)
		{
			if (m_Slots[i] == name)
				return static_cast<T>(1 << i); // Return the exact bit for this slot
		}

		return 0; // Not found
	}

	template<typename T>
	void FilterManager<T>::SetFilterNameAtSlot(uint32_t index, const std::string& name)
	{
		if (isSlotLocked(index))
		{
			EB_CORE_ERROR("Filter slot is locked! Name can't be set.");
			return;
		}

		if (index < 16)
			m_Slots[index] = name;
	}

	template<typename T>
	std::string FilterManager<T>::GetFilterNameBySlot(uint32_t index) const
	{
		if (index < 16)
			return m_Slots[index];
		return "";
	}

	template<typename T>
	void FilterManager<T>::InitWithFilters(const std::vector<std::string>& filters /* = {} */)
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

}