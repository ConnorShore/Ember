#pragma once

#pragma once

#include "Filter.h"

#include <string>
#include <vector>
#include <array>

namespace Ember {
	// TODO: Remove RenderLayer and CollisionFilter and replace with just "Filter"
	// Classes like project just make 2 filter managers and appropriate getters for it

	class FilterManager
	{
	public:
		FilterManager();

		void InitWithFilters(const std::vector<std::string>& filters = {});

		// UI/Editor Array Slot Access
		std::string GetFilterNameBySlot(uint32_t index) const;
		void SetFilterNameAtSlot(uint32_t index, const std::string& name);

		// Runtime Bitmask Access
		Filter GetFilter(const std::string& name) const;
		std::string GetFilterName(Filter filter) const;

		// Determine which slots are locked (can't be modified)
		inline bool isSlotLocked(uint16_t i) { return (1 << i) & m_Locked; }
		inline void setSlotLock(uint16_t slot, bool locked) { m_Locked |= ((int)locked) << slot; }

		// Bulk Queries
		std::vector<std::string> GetFilters() const;
		std::vector<std::string> GetActiveFilters(Filter activeFilter) const;

	private:
		// ReactPhysics3D uses a 16-bit integer mask, giving us exactly 16 fixed slots.
		std::string m_Slots[16];
		uint16_t m_Locked = 0;	// 16 bit integer masked for which slots are locked
	};

}