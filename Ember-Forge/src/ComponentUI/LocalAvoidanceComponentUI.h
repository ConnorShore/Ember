#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"

namespace Ember {

	class LocalAvoidanceComponentUI : public ComponentUI<LocalAvoidanceComponent>
	{
	public:
		LocalAvoidanceComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Local Avoidance Component"; }

	protected:
		inline void RenderComponentImpl(LocalAvoidanceComponent& component) override
		{
			if (UI::PropertyGrid::Begin("LocalAvoidanceProps"))
			{
				UI::PropertyGrid::Float("AvoidanceRadius", component.AvoidanceRadius);
				UI::PropertyGrid::Float("AvoidanceStrength", component.AvoidanceStrength);

				RenderFilterDropdown("Avoidance Mask", component.AvoidanceMask);

				UI::PropertyGrid::End();
			}
		}

	private:
		bool RenderFilterDropdown(const std::string& label, CollisionFilter& collisionFilter)
		{
			bool changed = false;
			auto& filterManager = ProjectManager::GetActive()->GetCollisionFilterManager();

			std::vector<std::string> activeFilterNames = filterManager.GetActiveFilters(collisionFilter);

			std::string name = "None";
			if (activeFilterNames.size() == 1)
				name = activeFilterNames[0];
			else if (activeFilterNames.size() > 1)
				name = "Multiple Filters";

			if (UI::PropertyGrid::BeginComboBox(label.c_str(), name))
			{
				std::vector<std::string> filterNames = filterManager.GetFilters();
				for (const auto& filterName : filterNames)
				{
					if (filterName == "Default")
						continue; // Skip the Default filter as it's always on by design

					CollisionFilter filterValue = filterManager.GetFilter(filterName);

					bool isSelected = (collisionFilter & filterValue) == filterValue;
					if (ImGui::Checkbox(filterName.c_str(), &isSelected))
					{
						if (isSelected)
							collisionFilter |= filterValue; // Set the bit
						else
							collisionFilter &= ~filterValue; // Unset the bit
						changed = true;
					}
				}

				ImGui::Separator();

				if (ImGui::Selectable("Clear All", false, ImGuiSelectableFlags_DontClosePopups))
				{
					collisionFilter = 0x0000; changed = true;
				}

				if (ImGui::Selectable("Select All", false, ImGuiSelectableFlags_DontClosePopups))
				{
					collisionFilter = 0xFFFF; changed = true;
				}

				UI::PropertyGrid::EndComboBox();
			}

			return changed;
		}
	};

}