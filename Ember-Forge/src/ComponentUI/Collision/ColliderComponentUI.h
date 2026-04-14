#pragma once

#include "ComponentUI/ComponentUI.h"

#include <Ember/Core/ProjectManager.h>

namespace Ember {

	template<typename T>
	class ColliderComponentUI : public ComponentUI<T>
	{
	public:
		ColliderComponentUI(EditorContext* context) : ComponentUI<T>(context) {}

		void CreateComponentForEntity(Entity entity) override
		{
			if (entity.ContainsComponent<T>())
			{
				EB_CORE_WARN("Entity already contains a component of type %s!", this->GetName());
				return;
			}

			T comp;
			this->m_Context->ActiveScene->AttachComponent(entity, comp);
		}

	protected:
		virtual void RenderComponentProperties(T& component) = 0;

	protected:
		inline void RenderComponentImpl(T& component) override
		{
			RenderComponentProperties(component);

			ImGui::PushID(&component);
			if (ImGui::TreeNode("Collision Filters"))
			{
				if (UI::PropertyGrid::Begin("CollisionFilterProps"))
				{
					RenderFilterDropdown("Category", component.Category);
					RenderFilterDropdown("Collision Mask", component.CollisionMask);

					UI::PropertyGrid::End();
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		}

	private:
		void RenderFilterDropdown(const std::string& label, Ember::CollisionFilter& collisionFilter)
		{
			auto& filterManager = Ember::ProjectManager::GetActive()->GetCollisionFilterManager();

			std::vector<std::string> activeFilterNames = filterManager.GetActiveFilters(collisionFilter);

			std::string name = "None";
			if (activeFilterNames.size() == 1)
				name = activeFilterNames[0];
			else if (activeFilterNames.size() > 1)
				name = "Multiple Filters";

			if (UI::PropertyGrid::BeginComboBox(label.c_str(), name))
			{
				std::vector<std::string> filterNames = filterManager.GetCustomFilters();
				for (const auto& filterName : filterNames)
				{
					
					Ember::CollisionFilter filterValue = filterManager.GetFilter(filterName);

					bool isSelected = (collisionFilter & filterValue) == filterValue;
					if (ImGui::Checkbox(filterName.c_str(), &isSelected))
					{
						if (isSelected)
							collisionFilter |= filterValue; // Set the bit
						else
							collisionFilter &= ~filterValue; // Unset the bit
					}
				}

				ImGui::Separator();

				// Optional: Setup quick actions for common masks
				if (ImGui::Selectable("Clear All", false, ImGuiSelectableFlags_DontClosePopups))
					collisionFilter = 0x0000; // All bits off

				if (ImGui::Selectable("Select All", false, ImGuiSelectableFlags_DontClosePopups))
					collisionFilter = 0xFFFF; // All 16 bits on

				UI::PropertyGrid::EndComboBox();
			}
		}
	};

}
