#include "Nodes.h"

#include <imgui/imgui.h>

namespace Ember {
	namespace UI::Nodes {

		bool BeginExpandableNode(const std::string& title, bool showRemove /*= false*/, UICallbackFunc onRemoveFunc /*= nullptr*/)
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
			if (showRemove)
			{
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
			}

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