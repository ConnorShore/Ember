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
		static constexpr uint32_t MaxSlots = 16;

		FilterManager();

		void InitWithFilters(const std::array<std::string, MaxSlots>& filters = {});

		// UI/Editor Array Slot Access
		std::string GetFilterNameBySlot(uint32_t index) const;
		void SetFilterNameAtSlot(uint32_t index, const std::string& name);

		// Runtime Bitmask Access
		Filter GetFilter(const std::string& name) const;
		std::string GetFilterName(Filter filter) const;

		// Bulk Queries
		std::vector<std::string> GetFilters() const;
		std::vector<std::string> GetActiveFilters(Filter activeFilter) const;
		const std::array<std::string, MaxSlots>& GetAllSlots() const;

	private:
		// ReactPhysics3D uses a 16-bit integer mask, giving us exactly 16 fixed slots.
		std::array<std::string, MaxSlots> m_Slots;
	};

}