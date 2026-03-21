#pragma once

#include "ComponentUI.h"

namespace Ember {

	class PointLightComponentUI : public ComponentUI<PointLightComponent>
	{
	public:
		PointLightComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Point Light Component"; }

	protected:
		inline void RenderComponentImpl(PointLightComponent& component) override
		{
			if (ImGui::BeginTable("PointLightProps", 2, ImGuiTableFlags_SizingFixedSame))
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
				ImGui::DragFloat("##Intensity", &component.Intensity, 1.0f, 0.0f, m_MaxValue, "%.2f");

				// Radius
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Use Radius");
				ImGui::TableNextColumn();
				if (ImGui::Checkbox("##UseRadius", &m_UseRadius))
				{
					if (m_UseRadius)
						component.Radius = 10.0f;
					else
						component.Radius = 0.0f;
				}

				if (m_UseRadius)
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Cutoff Radius");
					ImGui::TableNextColumn();
					ImGui::PushItemWidth(-FLT_MIN);
					ImGui::DragFloat("##Cutoff_Radius", &component.Radius, 0.5f, 0.001f, m_MaxValue, "%.2f");
				}

				ImGui::EndTable();
			}
		}

	private:
		bool m_UseRadius = false;
		float m_MaxValue = 999999.9f;
	};

}