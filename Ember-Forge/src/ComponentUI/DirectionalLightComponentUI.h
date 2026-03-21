#pragma once

#include "ComponentUI.h"

namespace Ember {

	class DirectionalLightComponentUI : public ComponentUI<DirectionalLightComponent>
	{
	public:
		DirectionalLightComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Directional Light Component"; }

	protected:
		inline void RenderComponentImpl(DirectionalLightComponent& component) override
		{
			if (ImGui::BeginTable("DirectionalLightProps", 2, ImGuiTableFlags_SizingFixedSame))
			{
				ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

				// Color
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Color");
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(-FLT_MIN);
				ImGui::ColorEdit3("##Color", &component.Color[0]);

				// Intensity
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Intensity");
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(-FLT_MIN);
				ImGui::DragFloat("##Intensity", &component.Intensity, 0.1f, 0.0f, m_MaxValue, "%.2f");

				ImGui::EndTable();
			}
		}

	private:
		float m_MaxValue = 1000.0f;
	};

}