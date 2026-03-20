#pragma once

#include "ComponentUI.h"

namespace Ember {

	class TransformComponentUI : public ComponentUI<TransformComponent>
	{
	public:
		TransformComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
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
				ImGui::DragFloat3("##Position", &component.Position[0], 0.1f, 0.0f, 0.0f, "%.2f", ImGuiSliderFlags_ColorMarkers);

				// Rotation
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Rotation");
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(-FLT_MIN);

				Vector3f rotationDegrees = Vector3f(
					Math::Degrees(component.Rotation.x),
					Math::Degrees(component.Rotation.y),
					Math::Degrees(component.Rotation.z)
				);
				if (ImGui::DragFloat3("##Rotation", &rotationDegrees[0], 1.0f, 0.0f, 0.0f, "%.1f", ImGuiSliderFlags_ColorMarkers))
				{
					component.Rotation = Vector3f(
						Math::Radians(rotationDegrees.x),
						Math::Radians(rotationDegrees.y),
						Math::Radians(rotationDegrees.z)
					);
				}

				// Scale
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Scale");
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(-FLT_MIN);
				ImGui::DragFloat3("##Scale", &component.Scale[0], 0.1f, 0.0f, 0.0f, "%.2f", ImGuiSliderFlags_ColorMarkers);

				ImGui::EndTable();
			}
		}
	};

}