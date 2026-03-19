#pragma once

#include "ComponentUI.h"

namespace Ember {

	class TransformComponentUI : public ComponentUI<TransformComponent>
	{
	public:
		
		inline const char* GetName() const override { return "Transform Component"; }

	protected:
		inline void RenderComponentImpl(TransformComponent& component) override
		{
			if (ImGui::BeginTable("TransformProps", 2, ImGuiTableFlags_SizingFixedSame))
			{
				ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

				// Position
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Position");
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(-FLT_MIN);
				ImGui::DragFloat3("##Position", &component.Position[0], m_SliderInterval, 0.0f, 0.0f, "%.2f", ImGuiSliderFlags_ColorMarkers);

				// Rotation
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Rotation");
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(-FLT_MIN);
				ImGui::DragFloat3("##Rotation", &component.Rotation[0], m_SliderInterval, 0.0f, 0.0f, "%.2f", ImGuiSliderFlags_ColorMarkers);

				// Scale
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Scale");
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(-FLT_MIN);
				ImGui::DragFloat3("##Scale", &component.Scale[0], m_SliderInterval, 0.0f, 0.0f, "%.2f", ImGuiSliderFlags_ColorMarkers);

				ImGui::EndTable();
			}
		}

	private:
		float m_SliderInterval = 0.1f;
	};

}