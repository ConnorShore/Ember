#pragma once

#include "CollisionFilter.h"
#include <string>
#include <vector>
#include <array>

namespace Ember {

	class CollisionFilterManager
	{
	public:
		CollisionFilterManager();

		void InitWithCustomFilters(const std::vector<std::string>& customFilters);
		void InitDefaultFilters();

		// UI/Editor Array Slot Access
		std::string GetFilterNameBySlot(uint32_t index) const;
		void SetFilterNameAtSlot(uint32_t index, const std::string& name);

		// Runtime Bitmask Access
		CollisionFilter GetFilter(const std::string& name) const;
		std::string GetFilterName(CollisionFilter filter) const;

		// Bulk Queries
		std::vector<std::string> GetFilters() const;
		std::vector<std::string> GetCustomFilters() const;
		std::vector<std::string> GetActiveFilters(CollisionFilter activeFilter) const;

	private:
		// ReactPhysics3D uses a 16-bit integer mask, giving us exactly 16 fixed slots.
		std::string m_Slots[16];
	};

}