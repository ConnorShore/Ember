#pragma once

#include "ComponentUI.h"

namespace Ember {

	class SpotLightComponentUI : public ComponentUI<SpotLightComponent>
	{
	public:
		SpotLightComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Spot Light Component"; }

	protected:
		inline void RenderComponentImpl(SpotLightComponent& component) override
		{
			if (ImGui::BeginTable("SpotLightProps", 2, ImGuiTableFlags_SizingFixedSame))
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

				float innerCutDegrees = Math::Degrees(component.CutOffAngle);
				float outerCutDegrees = Math::Degrees(component.OuterCutOffAngle);

				// Inner Cut Off Angle
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Inner Cut Off Angle");
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(-FLT_MIN);

				if (ImGui::DragFloat("##InnerCutOffAngle", &innerCutDegrees, 0.5f, 0.0f, outerCutDegrees - 0.1f, "%.2f"))
				{
					component.CutOffAngle = Math::Radians(innerCutDegrees);
					component.CutOff = cos(component.CutOffAngle);
				}

				// Outer Cut Off Angle
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Outer Cut Off Angle");
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(-FLT_MIN);

				if (ImGui::DragFloat("##OuterCutOffAngle", &outerCutDegrees, 0.5f, innerCutDegrees + 0.1f, 89.9f, "%.2f"))
				{
					component.OuterCutOffAngle = Math::Radians(outerCutDegrees);
					component.OuterCutOff = cos(component.OuterCutOffAngle);
				}

				ImGui::EndTable();
			}
		}

	private:
		float m_MaxValue = 999999.9f;
	};

}