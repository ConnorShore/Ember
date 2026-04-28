#include "efpch.h"
#include "Nodes.h"

#include <imgui/imgui.h>

namespace Ember {
	namespace UI::Nodes {

		bool BeginExpandableNode(const std::string& title)
		{
			const ImGuiTreeNodeFlags treeNodeFlags =
				ImGuiTreeNodeFlags_DefaultOpen |
				ImGuiTreeNodeFlags_Framed |
				ImGuiTreeNodeFlags_SpanAvailWidth |
				ImGuiTreeNodeFlags_AllowOverlap |
				ImGuiTreeNodeFlags_FramePadding;

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			bool ret = ImGui::TreeNodeEx(title.c_str(), treeNodeFlags, title.c_str());
			ImGui::PopStyleVar();

			return ret;
		}

		bool BeginEnabledExpandableNode(const std::string& title, bool& enabled, UICallbackFunc callbackFunc /* = nullptr */)
		{
			const ImGuiTreeNodeFlags treeNodeFlags =
				//ImGuiTreeNodeFlags_DefaultOpen |
				ImGuiTreeNodeFlags_Framed |
				ImGuiTreeNodeFlags_SpanAvailWidth |
				ImGuiTreeNodeFlags_AllowOverlap |
				ImGuiTreeNodeFlags_FramePadding;

			// Push padding for the Tree Node frame to make it thick and clickable
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });

			// Use fixed spacing to push the text label to the right, making room for our custom checkbox
			std::string paddedTitle = "     " + title;

			// Draw the tree node
			ImGui::BeginDisabled(!enabled);
			bool isOpen = ImGui::TreeNodeEx(title.c_str(), treeNodeFlags, paddedTitle.c_str());
			ImGui::EndDisabled();

			// Pop the padding so it doesn't affect the checkbox we are about to draw
			ImGui::PopStyleVar();

			// Get the exact screen coordinates of the header we just drew
			ImVec2 itemMin = ImGui::GetItemRectMin();
			ImVec2 itemMax = ImGui::GetItemRectMax();
			float headerHeight = itemMax.y - itemMin.y;

			// Shrink the checkbox AND add a border! 
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 0, 0 });
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
			float checkSize = ImGui::GetFrameHeight();

			// Calculate perfect mathematical centering
			float centerY = itemMin.y + (headerHeight - checkSize) * 0.5f;
			float offsetX = itemMin.x + ImGui::GetFontSize() + 13.0f;

			// Move the cursor to our exact calculated coordinates
			ImGui::SetCursorScreenPos(ImVec2(offsetX, centerY));

			// Draw the checkbox
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			if (ImGui::Checkbox(("##" + title).c_str(), &enabled) && callbackFunc)
			{
				callbackFunc();
			}
			ImGui::PopStyleColor();
			ImGui::PopStyleVar(2);

			return isOpen;
		}

		bool BeginRemoveableExpandableNode(const std::string& title, UICallbackFunc onRemoveFunc /*= nullptr*/)
		{
			const ImGuiTreeNodeFlags treeNodeFlags =
				ImGuiTreeNodeFlags_DefaultOpen |
				ImGuiTreeNodeFlags_Framed |
				ImGuiTreeNodeFlags_SpanAvailWidth |
				ImGuiTreeNodeFlags_AllowOverlap |
				ImGuiTreeNodeFlags_FramePadding;

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			bool ret = ImGui::TreeNodeEx(title.c_str(), treeNodeFlags, title.c_str());
			ImGui::PopStyleVar();

			bool removed = false;
			float buttonWidth = ImGui::CalcTextSize("Remove").x + ImGui::GetStyle().FramePadding.x * 2.0f;
			ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - buttonWidth - 5.0f);

			// Center button vertically with the header text
			float currentCursorY = ImGui::GetCursorPosY();
			ImGui::SetCursorPosY(currentCursorY + 2.0f);

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 2 });
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));

			std::string buttonID = "Remove##" + title;
			if (ImGui::Button(buttonID.c_str()))
			{
				removed = true;
			}

			ImGui::PopStyleColor(1);
			ImGui::PopStyleVar(3);

			if (removed && onRemoveFunc)
			{
				onRemoveFunc();
			}

			return ret;
		}

		void EndExpandableNode()
		{
			ImGui::TreePop();
		}
	}
}