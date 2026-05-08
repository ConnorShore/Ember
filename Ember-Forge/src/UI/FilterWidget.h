#pragma once

#include <Ember/Core/FilterManager.h>

#include <imgui/imgui.h>
#include <string>

namespace Ember::UI {

	inline bool RenderFilterGrid(const std::string& id, Filter& filter, FilterManager& filterManager)
	{
		ImGui::PushID(id.c_str());

		bool changed = false;

		// Note: Depending on your UI::PropertyGrid wrapper, you may need to manually
		// draw the label in the left column here using ImGui::Text() and ImGui::NextColumn() 
		// since we are no longer using BeginComboBox.

		ImGui::BeginGroup();

		ImVec2 buttonSize(22, 22);
		ImVec2 spacing(2, 2);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, spacing);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));

		// Godot organizes its 16 bits in two visual blocks of 2x4.
		// We map our loop index to the actual bit indices to replicate this UI layout perfectly.
		int godotLayout[16] = {
			0, 1, 2, 3,   8,  9, 10, 11,  // Row 1 (Displays: 1-4, 9-12)
			4, 5, 6, 7,  12, 13, 14, 15   // Row 2 (Displays: 5-8, 13-16)
		};

		for (int i = 0; i < 16; ++i)
		{
			int bitIndex = godotLayout[i];
			Filter bitMask = static_cast<Filter>(1 << bitIndex);
			bool isActive = (filter & bitMask) != 0;

			// FAKE TOGGLE BUTTON STYLING
			if (isActive)
			{
				// Active state (Godot Blue)
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.9f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.3f, 0.7f, 1.0f));
			}
			else
			{
				// Inactive state (Dark Grey)
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
			}

			ImGui::PushID(bitIndex);
			std::string btnLabel = std::to_string(bitIndex + 1);
			if (ImGui::Button(btnLabel.c_str(), buttonSize))
			{
				// Toggle the specific bit using XOR
				filter ^= bitMask;
				changed = true;
			}
			ImGui::PopID();
			ImGui::PopStyleColor(3);

			// Handle row wrapping and the Godot middle gap
			if ((i + 1) % 8 != 0)
			{
				ImGui::SameLine();
				if ((i + 1) % 4 == 0)
				{
					// Add a slightly larger gap to separate the two 4-button blocks
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
				}
			}
		}

		ImGui::PopStyleVar(3);
		ImGui::EndGroup();

		// --- THE 3-DOT MENU ---
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);

		// Span the button height across both rows (22 + 2 + 22 = 46)
		if (ImGui::Button("...", ImVec2(24, 46)))
		{
			ImGui::OpenPopup("FilterMenuPopup");
		}

		// --- THE POPUP CONTEXT ---
		if (ImGui::BeginPopup("FilterMenuPopup"))
		{
			std::vector<std::string> filterNames = filterManager.GetFilters();

			// Render the checkboxes exactly as you had them
			for (const auto& filterName : filterNames)
			{
				Filter filterValue = filterManager.GetFilter(filterName);
				bool isSelected = (filter & filterValue) == filterValue;

				if (ImGui::Checkbox(filterName.c_str(), &isSelected))
				{
					if (isSelected)
						filter |= filterValue;
					else
						filter &= ~filterValue;
					changed = true;
				}
			}

			if (!filterNames.empty())
				ImGui::Separator();

			if (ImGui::Selectable("Clear All", false, ImGuiSelectableFlags_DontClosePopups))
			{
				filter = 0x0000;
				changed = true;
			}

			if (ImGui::Selectable("Select All", false, ImGuiSelectableFlags_DontClosePopups))
			{
				filter = 0xFFFF;
				changed = true;
			}

			// (Optional) Add your layer editing transition here
			// ImGui::Separator();
			// if (ImGui::Selectable("Edit Layer Names")) { /* Open project settings window */ }

			ImGui::EndPopup();
		}

		ImGui::PopID();

		return changed;
	}

}

//#pragma once
//
//#include <Ember/Core/FilterManager.h>
//
//#include <string>
//
//namespace Ember::UI {
//
//	bool RenderFilterDropdown(const std::string& label, Filter& filter, FilterManager& filterManager)
//	{
//		bool changed = false;
//		std::vector<std::string> activeFilterNames = filterManager.GetActiveFilters(filter);
//
//		std::string name = "None";
//		if (activeFilterNames.size() == 1)
//			name = activeFilterNames[0];
//		else if (activeFilterNames.size() > 1)
//			name = "Multiple Filters";
//
//		if (UI::PropertyGrid::BeginComboBox(label.c_str(), name))
//		{
//			std::vector<std::string> filterNames = filterManager.GetFilters();
//			for (const auto& filterName : filterNames)
//			{
//				Filter filterValue = filterManager.GetFilter(filterName);
//
//				bool isSelected = (filter & filterValue) == filterValue;
//				if (ImGui::Checkbox(filterName.c_str(), &isSelected))
//				{
//					if (isSelected)
//						filter |= filterValue; // Set the bit
//					else
//						filter &= ~filterValue; // Unset the bit
//					changed = true;
//				}
//			}
//
//			ImGui::Separator();
//
//			if (ImGui::Selectable("Clear All", false, ImGuiSelectableFlags_DontClosePopups))
//			{
//				filter = 0x0000; changed = true;
//			}
//
//			if (ImGui::Selectable("Select All", false, ImGuiSelectableFlags_DontClosePopups))
//			{
//				filter = 0xFFFF; changed = true;
//			}
//
//			UI::PropertyGrid::EndComboBox();
//		}
//
//		return changed;
//	}
//
//}