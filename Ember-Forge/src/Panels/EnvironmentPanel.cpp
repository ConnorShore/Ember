#include "efpch.h"
#include "EnvironmentPanel.h"
#include "UI/Nodes.h"
#include "UI/PropertyGrid.h"
#include <Ember/Render/VFX/BloomPass.h>
#include <Ember/ECS/System/RenderSystem.h>

namespace Ember {


	EnvironmentPanel::EnvironmentPanel(EditorContext* context)
		: Panel("Environment", context)
	{

	}

	void EnvironmentPanel::OnImGuiRender()
	{
		ImGui::Begin("Environment");

		if (UI::Nodes::BeginExpandableNode("Bloom"))
		{
			auto bloomPass = StaticPointerCast<BloomPass>(Application::Instance().GetSystem<RenderSystem>()->GetPostProcessPass<BloomPass>());
			ImGui::Checkbox("Enable", &bloomPass->Enabled);

			if (UI::PropertyGrid::Begin("##BloomPropertyGrid"))
			{
				ImGui::BeginDisabled(!bloomPass->Enabled);

				UI::PropertyGrid::Float("Threshold", bloomPass->Threshold, 0.01f, 0.0f, 10.0f);
				UI::PropertyGrid::Float("Soft Knee", bloomPass->Knee, 0.01f, 0.0f, 1.0f);
				UI::PropertyGrid::Float("Intensity", bloomPass->Intensity, 0.01f, 0.0f, 5.0f);
				UI::PropertyGrid::Float("Blur Radius", bloomPass->BlurRadius, 0.01f, 0.1f, 5.0f);

				ImGui::EndDisabled();

				UI::PropertyGrid::End();
			}


			UI::Nodes::EndExpandableNode();
		}
		
		ImGui::End();
	}

}