#pragma once

#include "ComponentUI.h"

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
			if (ImGui::BeginTable("TransformProps", 2, ImGuiTableFlags_SizingFixedSame))
			{
				ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

				// Mesh
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Mesh:");
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(-FLT_MIN);

				const char* meshName = component.Mesh ? component.Mesh->GetName().c_str() : "None";
				ImGui::Text(meshName);
				ImGui::SameLine();

				if (ImGui::Button("Select Mesh"))
				{
					ImGui::OpenPopup("ChooseMeshPopup");
				}

				if (ImGui::BeginPopup("ChooseMeshPopup"))
				{
					std::string name = m_Context->SelectedEntity.GetComponent<TagComponent>().Tag + "_Mesh";
					if (ImGui::MenuItem("Cube"))
					{
						component.Mesh = m_Context->ActiveScene->GetAsset<Mesh>("Primitive_Cube");
					}
					if (ImGui::MenuItem("Quad"))
					{
						component.Mesh = m_Context->ActiveScene->GetAsset<Mesh>("Primitive_Quad");
					}
					if (ImGui::MenuItem("Sphere"))
					{
						component.Mesh = m_Context->ActiveScene->GetAsset<Mesh>("Primitive_Sphere");
					}

					ImGui::EndPopup();
				}

				ImGui::EndTable();
			}
		}

	private:
	};

}