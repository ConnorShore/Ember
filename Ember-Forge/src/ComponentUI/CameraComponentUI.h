#pragma once

#include "ComponentUI.h"

namespace Ember {

	class CameraComponentUI : public ComponentUI<CameraComponent>
	{
	public:
		CameraComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Camera Component"; }

	protected:
		inline void RenderComponentImpl(CameraComponent& component) override
		{
			auto& camera = component.Camera;

			// Checkbox for active state
			ImGui::Checkbox("Active", &component.IsActive);

			// Dropdown for projection type
			ImGui::Text("Projection:");
			ImGui::SameLine();

			std::string projectionTypeName = Camera::GetProjectionTypeName(camera.GetProjectionType());
			if (ImGui::BeginCombo("##CameraCombo", projectionTypeName.c_str()))
			{
				for (unsigned int i = 0; i < static_cast<int>(Camera::ProjectionType::Count); i++)
				{
					auto type = static_cast<Camera::ProjectionType>(i);
					std::string typeName = Camera::GetProjectionTypeName(type);

					bool isSelected = typeName == projectionTypeName;
					if (ImGui::Selectable(typeName.c_str(), isSelected))
						camera.SetProjectionType(type);

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			ImGui::Separator();
			ImGui::Text("Properties");
			if (ImGui::BeginTable("TransformProps", 2, ImGuiTableFlags_SizingFixedSame))
			{
				ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

				if (camera.GetProjectionType() == Camera::ProjectionType::Perspective)
				{
					// FOV
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Field Of View");
					ImGui::TableNextColumn();
					ImGui::PushItemWidth(-FLT_MIN);
					ImGui::DragFloat("##FieldOfView", &camera.GetPerspectiveProps().FieldOfView, 0.1f, 0.0f, 0.0f, "%.2f");

					// Near Clip
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Near Clip");
					ImGui::TableNextColumn();
					ImGui::PushItemWidth(-FLT_MIN);
					ImGui::DragFloat("##NearClip", &camera.GetPerspectiveProps().NearClip, 0.1f, 0.0f, 0.0f, "%.2f");

					// FOV
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Far Clip");
					ImGui::TableNextColumn();
					ImGui::PushItemWidth(-FLT_MIN);
					ImGui::DragFloat("##FarClip", &camera.GetPerspectiveProps().FarClip, 0.1f, 0.0f, 0.0f, "%.2f");

				}
				else if (camera.GetProjectionType() == Camera::ProjectionType::Orthographic)
				{
					// FOV
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Size");
					ImGui::TableNextColumn();
					ImGui::PushItemWidth(-FLT_MIN);
					ImGui::DragFloat("##Size", &camera.GetOrthographicProps().Size, 0.1f, 0.0f, 0.0f, "%.2f");

					// Near Clip
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Near Clip");
					ImGui::TableNextColumn();
					ImGui::PushItemWidth(-FLT_MIN);
					ImGui::DragFloat("##NearClip", &camera.GetOrthographicProps().NearClip, 0.1f, 0.0f, 0.0f, "%.2f");

					// FOV
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::AlignTextToFramePadding();
					ImGui::Text("Far Clip");
					ImGui::TableNextColumn();
					ImGui::PushItemWidth(-FLT_MIN);
					ImGui::DragFloat("##FarClip", &camera.GetOrthographicProps().FarClip, 0.1f, 0.0f, 0.0f, "%.2f");
				}
				else
				{
					EB_CORE_ASSERT(false, "Unsupported projection type selected!");
				}

				
				ImGui::EndTable();
			}

			
		}

	private:
	};

}