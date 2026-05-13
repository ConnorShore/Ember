#pragma once

#include "ComponentUI.h"
#include "UI/UIWidgets.h"
#include "UI/PropertyGrid.h"
#include "UI/FilterWidget.h"

#include <Ember/Core/ProjectManager.h>

namespace Ember {

	class CameraComponentUI : public ComponentUI<CameraComponent>
	{
	public:
		CameraComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Camera Component"; }

		virtual void CreateComponentForEntity(Entity entity) override
		{
			bool isFirstCamera = false;
			if (auto scene = m_Context->ActiveScene())
			{
				if (scene->GetAllEntitiesWithComponents<CameraComponent>().size() == 0)
					isFirstCamera = true;
			}

			auto& cameraComponent = entity.AttachComponent<CameraComponent>();
			cameraComponent.IsActive = isFirstCamera;
		}

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
			if (UI::BeginComboBox("##CameraCombo", projectionTypeName.c_str()))
			{
				for (uint32_t i = 0; i < static_cast<uint32_t>(Camera::ProjectionType::Count); i++)
				{
					auto type = static_cast<Camera::ProjectionType>(i);
					std::string typeName = Camera::GetProjectionTypeName(type);

					bool isSelected = typeName == projectionTypeName;
					if (UI::ComboBoxItem(typeName.c_str(), isSelected))
						camera.SetProjectionType(type);
				}
				UI::EndComboBox();
			}

			ImGui::Separator();
			ImGui::Text("Properties");
			if (UI::PropertyGrid::Begin("TransformProps"))
			{
				if (camera.GetProjectionType() == Camera::ProjectionType::Perspective)
				{
					// FOV
					UI::PropertyGrid::Float("Field Of View", camera.GetPerspectiveProps().FieldOfView);
					
					// Near Clip
					UI::PropertyGrid::Float("Near Clip", camera.GetPerspectiveProps().NearClip);

					// Far Clip
					UI::PropertyGrid::Float("Far Clip", camera.GetPerspectiveProps().FarClip);
				}
				else
				{
					// Size
					UI::PropertyGrid::Float("Size", camera.GetOrthographicProps().Size);

					// Near Clip
					UI::PropertyGrid::Float("Near Clip", camera.GetOrthographicProps().NearClip);

					// Far Clip
					UI::PropertyGrid::Float("Far Clip", camera.GetOrthographicProps().FarClip);
				}

				UI::PropertyGrid::End();
			}

			if (ImGui::TreeNode("Camera Masks"))
			{
				if (UI::PropertyGrid::Begin("CameraMaskProps"))
				{
					UI::PropertyGrid::FilterGrid("Render Mask", component.RenderMask, ProjectManager::GetActive()->GetRenderFilterManager());
					UI::PropertyGrid::FilterGrid("Volume Mask", component.VolumeMask, ProjectManager::GetActive()->GetCollisionFilterManager());
					UI::PropertyGrid::End();
				}

				ImGui::TreePop();
			}
		}

	private:
	};

}