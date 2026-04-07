#include "efpch.h"
#include "EnvironmentPanel.h"
#include "UI/Nodes.h"
#include "UI/PropertyGrid.h"

#include <Ember/Render/VFX/BloomPass.h>
#include <Ember/ECS/System/RenderSystem.h>
#include <Ember/Event/UIEvent.h>

namespace Ember {


	EnvironmentPanel::EnvironmentPanel(EditorContext* context)
		: Panel("Environment", context)
	{

	}

	void EnvironmentPanel::OnImGuiRender()
	{
		ImGui::Begin("Environment");

		RenderSkyboxSettings();
		RenderBloomSettings();
		
		ImGui::End();
	}

	void EnvironmentPanel::RenderSkyboxSettings()
	{
		if (UI::Nodes::BeginExpandableNode("Skybox"))
		{
			auto skybox = Application::Instance().GetSystem<RenderSystem>()->GetSkybox();
			bool enabled = skybox->Enabled();
			if (ImGui::Checkbox("Enable", &enabled))
				skybox->SetEnabled(enabled);

			if (UI::PropertyGrid::Begin("##SkyboxPropertyGrid"))
			{
				ImGui::BeginDisabled(!skybox->Enabled());

				// Texture Drop
				std::string droppedFilePath;
				if (UI::PropertyGrid::DragDropTexture("Skybox Texture", skybox->GetSkyboxTextureHandle(), droppedFilePath, [&]() {
					skybox->Initialize(Constants::Assets::DefaultSkyboxUUID);	// TODO: Move to constant
				}))
				{
					auto newSkyboxTex = Application::Instance().GetAssetManager().Load<Texture2D>(droppedFilePath);
					skybox->Initialize(newSkyboxTex->GetUUID());

					auto evt = UINotificationEvent(std::format("Skybox texture updated to {}", droppedFilePath));
					m_Context->EventCallback(evt);
				}

				// Intensity Slider
				float intensity = skybox->GetIntensity();
				if (UI::PropertyGrid::Float("Ambient Intensity", intensity, 0.01f, 0.0f, 10.0f))
					skybox->SetIntensity(intensity);

				ImGui::EndDisabled();
				UI::PropertyGrid::End();
			}


			UI::Nodes::EndExpandableNode();
		}
	}

	void EnvironmentPanel::RenderBloomSettings()
	{
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
	}

}