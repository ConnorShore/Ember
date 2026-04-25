#include "efpch.h"
#include "EnvironmentPanel.h"
#include "UI/Nodes.h"
#include "UI/PropertyGrid.h"

#include <Ember/Render/VFX/BloomPass.h>
#include <Ember/Render/VFX/FXAAPass.h>
#include <Ember/Render/VFX/ColorGradePass.h>
#include <Ember/Render/VFX/ToneMapPass.h>
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
		RenderFXAASettings();
		RenderColorGradeSettings();
		
		ImGui::End();
	}

	void EnvironmentPanel::RenderSkyboxSettings()
	{
		auto skybox = Application::Instance().GetSystem<RenderSystem>()->GetSkybox();
		bool enabled = skybox->Enabled();

		if (UI::Nodes::BeginEnabledExpandableNode("Skybox", enabled, [&]() {
			skybox->SetEnabled(enabled);
		}))
		{
			if (UI::PropertyGrid::Begin("##SkyboxPropertyGrid"))
			{
				ImGui::BeginDisabled(!skybox->Enabled());

				// Texture Drop
				std::string droppedFilePath;
				if (UI::PropertyGrid::DragDropTexture("Skybox Texture", skybox->GetSkyboxTextureHandle(), droppedFilePath, [&]() {
					skybox->Initialize(Constants::Assets::DefaultSkyboxUUID);	// TODO: Move to constant
				}))
				{
					std::string skyboxTexName = std::filesystem::path(droppedFilePath).stem().string();
					auto newSkyboxTex = Application::Instance().GetAssetManager().Load<Texture2D>(UUID(), skyboxTexName, droppedFilePath, false);
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
		auto bloomPass = StaticPointerCast<BloomPass>(Application::Instance().GetSystem<RenderSystem>()->GetPostProcessPass<BloomPass>());
		if (UI::Nodes::BeginEnabledExpandableNode("Bloom", bloomPass->Enabled))
		{
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

	void EnvironmentPanel::RenderFXAASettings()
	{
		auto fxaaPass = StaticPointerCast<FXAAPass>(Application::Instance().GetSystem<RenderSystem>()->GetPostProcessPass<FXAAPass>());
		if (UI::Nodes::BeginEnabledExpandableNode("FXAA", fxaaPass->Enabled))
		{
			if (UI::PropertyGrid::Begin("##FXAAPropertyGrid"))
			{
				ImGui::BeginDisabled(!fxaaPass->Enabled);
				UI::PropertyGrid::Float("Sub Pixel Quality", fxaaPass->SubpixelQuality, 0.001f, 0.0f, 1.0f);
				UI::PropertyGrid::Float("Edge Threshold Min", fxaaPass->EdgeThresholdMin, 0.0001f, 0.0f, 0.0833f, "%.4f");
				UI::PropertyGrid::Float("Edge Threshold Max", fxaaPass->EdgeThresholdMax, 0.001f, 0.063f, 0.333f, "%.4f");
				ImGui::EndDisabled();

				UI::PropertyGrid::End();
			}
			UI::Nodes::EndExpandableNode();
		}
	}

	void EnvironmentPanel::RenderColorGradeSettings()
	{
		auto colorGradePass = StaticPointerCast<ColorGradePass>(Application::Instance().GetSystem<RenderSystem>()->GetPostProcessPass<ColorGradePass>());
		if (UI::Nodes::BeginEnabledExpandableNode("Color Grading", colorGradePass->Enabled))
		{
			auto toneMapPass = StaticPointerCast<ToneMapPass>(Application::Instance().GetSystem<RenderSystem>()->GetPostProcessPass<ToneMapPass>());
			auto& colorGradeProps = colorGradePass->Settings;

			ImGui::BeginDisabled(!colorGradePass->Enabled);
			if (ImGui::TreeNode("Exposure"))
			{
				if (UI::PropertyGrid::Begin("##ExposureProps"))
				{
					UI::PropertyGrid::Float("Exposure", toneMapPass->Exposure, 0.01f, 0.0f, 10.0f);
					UI::PropertyGrid::End();
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("White Balance"))
			{
				if (UI::PropertyGrid::Begin("##WhiteBalanceProps"))
				{
					UI::PropertyGrid::Float("Temperature", colorGradeProps.Temperature, 0.01f, -1.0f, 1.0f);
					UI::PropertyGrid::Float("Tint", colorGradeProps.Tint, 0.01f, -1.0f, 1.0f);
					UI::PropertyGrid::End();
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Color Adjustments"))
			{
				if (UI::PropertyGrid::Begin("##ColorAdjustmentProps"))
				{
					UI::PropertyGrid::Float("Contrast", colorGradeProps.Contrast, 0.01f, 0.0f, 2.0f);
					UI::PropertyGrid::Float("Saturation", colorGradeProps.Saturation, 0.01f, 0.0f, 2.0f);

					UI::PropertyGrid::End();
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Lift, Gamma, Gain"))
			{
				if (UI::PropertyGrid::Begin("##LGGProps"))
				{
					UI::PropertyGrid::Color4("Lift", colorGradeProps.Lift);
					UI::PropertyGrid::Color4("Gamma", colorGradeProps.Gamma);
					UI::PropertyGrid::Color4("Gain", colorGradeProps.Gain);

					UI::PropertyGrid::End();
				}
				ImGui::TreePop();
			}
			ImGui::EndDisabled();

			UI::Nodes::EndExpandableNode();
		}
	}

}