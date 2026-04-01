#pragma once

#include "ComponentUI.h"
#include "UI/PropertyGrid.h"

#include <Ember.h>

namespace Ember {

	class MeshComponentUI : public ComponentUI<MeshComponent>
	{
	public:
		MeshComponentUI(EditorContext* context) : ComponentUI(context) {}
		inline const char* GetName() const override { return "Mesh Component"; }

	protected:
		inline void RenderComponentImpl(MeshComponent& component) override
		{
			if (UI::PropertyGrid::Begin("MeshComponentProps"))
			{
				if (UI::PropertyGrid::HeaderWithActionButton("Mesh", "Select Mesh", m_SelectedMeshName))
				{
					ImGui::OpenPopup("ChooseMeshPopup");
				}

				if (ImGui::BeginPopup("ChooseMeshPopup"))
				{
					std::string name = m_Context->SelectedEntity.GetComponent<TagComponent>().Tag + "_Mesh";
					if (ImGui::MenuItem("Cube"))
					{
						component.MeshHandle = Constants::Assets::CubeMeshUUID;
						m_SelectedMeshName = "Cube";
					}
					if (ImGui::MenuItem("Quad"))
					{
						component.MeshHandle = Constants::Assets::QuadMeshUUID;
						m_SelectedMeshName = "Quad";
					}
					if (ImGui::MenuItem("Sphere"))
					{
						component.MeshHandle = Constants::Assets::SphereMeshUUID;
						m_SelectedMeshName = "Sphere";
					}

					ImGui::EndPopup();
				}

				UI::PropertyGrid::End();
			}
		}

	private:
		std::string m_SelectedMeshName = "None";
	};

}