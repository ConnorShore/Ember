#include "EnvironmentPanel.h"

namespace Ember {


	EnvironmentPanel::EnvironmentPanel(EditorContext* context)
		: Panel("Environment", context)
	{

	}

	void EnvironmentPanel::OnImGuiRender()
	{
		ImGui::Begin("Environment");

		// Bloom section
		const ImGuiTreeNodeFlags treeNodeFlags =
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_Framed |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_AllowOverlap |
			ImGuiTreeNodeFlags_FramePadding;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
		bool open = ImGui::TreeNodeEx("Bloom", treeNodeFlags, "Bloom");
		ImGui::PopStyleVar();

		if (open)
		{
			auto bloomPass = Application::Instance().GetSystem<RenderSystem>()->GetPostProcessPass<BloomPass>();
			ImGui::Checkbox("Enable", &bloomPass->Enabled);
			ImGui::BeginDisabled(!bloomPass->Enabled);

			// TODO: Get this hooked up
			float test;
			ImGui::DragFloat("Threshold", &test, 0.01f, 0.0f, 10.0f, "%.2f");
			//ImGui::DragFloat("Threshold", &bloomPass->Threshold, 0.01f, 0.0f, 10.0f, "%.2f");
			//ImGui::DragFloat("Soft Knee", &bloomPass->Knee, 0.01f, 0.0f, 1.0f, "%.2f");
			//ImGui::DragFloat("Intensity", &bloomPass->Intensity, 0.01f, 0.0f, 5.0f, "%.2f");
			//ImGui::DragFloat("Blur Radius", &bloomPass->BlurRadius, 0.01f, 0.1f, 5.0f, "%.2f");
			
			ImGui::EndDisabled();
			
			ImGui::TreePop();
		}
		
		ImGui::End();
	}

}