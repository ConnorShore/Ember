#include "ebpch.h"
#include "CollisionFilterManager.h"

namespace Ember {

	CollisionFilterManager::CollisionFilterManager()
	{
		// Ensure the array is completely clear on startup
		for (int i = 0; i < 16; i++)
			m_Slots[i] = "";
	}

	void CollisionFilterManager::InitWithCustomFilters(const std::vector<std::string>& customFilters)
	{
		// Only set default as others may have been overridden by the user, we want to preserve those if possible
		m_Slots[0] = "Default";     // Bit 0: 0x0001

		uint32_t slotIndex = 1;
		for (const auto& filterName : customFilters)
		{
			if (slotIndex >= 16)
			{
				EB_CORE_WARN("Exceeded maximum number of collision filters (16). Skipping: {}", filterName);
				break;
			}
			m_Slots[slotIndex] = filterName;
			slotIndex++;
		}
	}

	void CollisionFilterManager::InitDefaultFilters()
	{
		// Lock the foundational layers to specific bits so they never shift
		m_Slots[0] = "Default";     // Bit 0: 0x0001
		m_Slots[1] = "Player";      // Bit 1: 0x0002
		m_Slots[2] = "Enemy";       // Bit 2: 0x0004
		m_Slots[3] = "Environment"; // Bit 3: 0x0008
	}

	std::string CollisionFilterManager::GetFilterNameBySlot(uint32_t index) const
	{
		if (index < 16)
			return m_Slots[index];
		return "";
	}

	void CollisionFilterManager::SetFilterNameAtSlot(uint32_t index, const std::string& name)
	{
		if (index > 0 && index < 16) // > 0 prevents overwriting the "Default" slot!
			m_Slots[index] = name;
	}

	CollisionFilter CollisionFilterManager::GetFilter(const std::string& name) const
	{
		if (name.empty()) return 0;

		for (int i = 0; i < 16; i++)
		{
			if (m_Slots[i] == name)
				return static_cast<CollisionFilter>(1 << i); // Return the exact bit for this slot
		}

		return 0; // Not found
	}

	std::string CollisionFilterManager::GetFilterName(CollisionFilter filter) const
	{
		for (int i = 0; i < 16; i++)
		{
			if (filter == static_cast<CollisionFilter>(1 << i))
				return m_Slots[i];
		}

		// If multiple bits are set, or it's empty
		return "Multiple";
	}

	std::vector<std::string> CollisionFilterManager::GetFilters() const
	{
		std::vector<std::string> names;
		for (int i = 0; i < 16; i++)
		{
			if (!m_Slots[i].empty())
				names.push_back(m_Slots[i]);
		}
		return names;
	}

	std::vector<std::string> CollisionFilterManager::GetCustomFilters() const
	{
		std::vector<std::string> names;
		for (int i = 1; i < 16; i++)
		{
			if (!m_Slots[i].empty())
				names.push_back(m_Slots[i]);
		}
		return names;
	}

	std::vector<std::string> CollisionFilterManager::GetActiveFilters(CollisionFilter activeFilter) const
	{
		std::vector<std::string> names;
		for (int i = 0; i < 16; i++)
		{
			if (m_Slots[i].empty())
				continue;

			// Extract the bit for the current slot and check if it's active in the mask
			CollisionFilter bit = static_cast<CollisionFilter>(1 << i);
			if ((activeFilter & bit) == bit)
			{
				names.push_back(m_Slots[i]);
			}
		}
		return names;
	}

}