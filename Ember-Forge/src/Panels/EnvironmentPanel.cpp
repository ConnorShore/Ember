#include "efpch.h"
#include "EnvironmentPanel.h"
#include "UI/Nodes.h"
#include "UI/PropertyGrid.h"

#include <Ember/Render/VFX/BloomPass.h>
#include <Ember/Render/VFX/FogPass.h>
#include <Ember/Render/VFX/FXAAPass.h>
#include <Ember/Render/VFX/ColorGradePass.h>
#include <Ember/Render/VFX/ToneMapPass.h>
#include <Ember/Render/VFX/VignettePass.h>
#include <Ember/ECS/System/RenderSystem.h>
#include <Ember/Core/ProjectManager.h>
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
		RenderFogSettings();
		RenderVignetteSettings();
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
		auto bloomPass = StaticPointerCast<BloomPass>(Application::Instance().GetSystem<RenderSystem>()->GetPostProcessPass("BloomPass"));
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

	void EnvironmentPanel::RenderFogSettings()
	{
		auto fogPass = StaticPointerCast<FogPass>(Application::Instance().GetSystem<RenderSystem>()->GetPostProcessPass("FogPass"));
		if (UI::Nodes::BeginEnabledExpandableNode("Fog", fogPass->Enabled))
		{
			if (UI::PropertyGrid::Begin("##FogPropertyGrid"))
			{
				ImGui::BeginDisabled(!fogPass->Enabled);
				UI::PropertyGrid::Color3("Color", fogPass->Color);
				UI::PropertyGrid::Float("Density", fogPass->Density, 0.001f, 0.0f, 1.0f);
				UI::PropertyGrid::Float("Start Distance", fogPass->StartDistance, 0.01f, 0.0f, 100000.0f);
				UI::PropertyGrid::Float("Falloff", fogPass->Falloff, 0.01f, 0.0f, 100000.0f);
				ImGui::EndDisabled();

				UI::PropertyGrid::End();
			}
			UI::Nodes::EndExpandableNode();
		}
	}

	void EnvironmentPanel::RenderFXAASettings()
	{
		auto fxaaPass = StaticPointerCast<FXAAPass>(Application::Instance().GetSystem<RenderSystem>()->GetPostProcessPass("FXAAPass"));
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

	void EnvironmentPanel::RenderVignetteSettings()
	{
		auto vignettePass = StaticPointerCast<VignettePass>(Application::Instance().GetSystem<RenderSystem>()->GetPostProcessPass("VignettePass"));
		if (UI::Nodes::BeginEnabledExpandableNode("Vignette", vignettePass->Enabled))
		{
			if (UI::PropertyGrid::Begin("##VignettePropertyGrid"))
			{
				ImGui::BeginDisabled(!vignettePass->Enabled);
				UI::PropertyGrid::Color3("Color", vignettePass->Color);
				UI::PropertyGrid::Float("Intensity", vignettePass->Intensity, 0.01f, 0.0f, 5.0f);
				UI::PropertyGrid::Float("Size", vignettePass->Size, 0.001f, 0.0f, 1.0f);
				UI::PropertyGrid::Float("Smoothness", vignettePass->Smoothness, 0.001f, 0.0f, 1.0f);
				ImGui::EndDisabled();

				UI::PropertyGrid::End();
			}
			UI::Nodes::EndExpandableNode();
		}
	}

	void EnvironmentPanel::RenderColorGradeLUTSettings(const SharedPtr<ColorGradePass>& colorGradePass, const SharedPtr<ToneMapPass>& toneMapPass)
	{
		ColorGradeSettings& colorGradeProps = colorGradePass->Settings;
		if (UI::PropertyGrid::Begin("##ColorGradePropertyGrid"))
		{
			// Texture Drop
			std::string droppedFilePath;
			if (UI::PropertyGrid::DragDropTexture("LUT Texture", colorGradePass->GetBaseBakedLUT()->GetUUID(), droppedFilePath, [&]() {
				auto neutralLUT = Application::Instance().GetAssetManager().GetAsset<Texture2D>(Constants::Assets::DefaultNeutralColorLUTUUID);
				colorGradePass->SetBaseBakedLUT(neutralLUT);
				}))
			{
				std::string lutTexName = std::filesystem::path(droppedFilePath).stem().string();
				auto newLUTTex = Application::Instance().GetAssetManager().Load<Texture2D>(UUID(), lutTexName, droppedFilePath, false);
				colorGradePass->SetBaseBakedLUT(newLUTTex);
			}

			UI::PropertyGrid::End();
		}

		ImGui::Separator();

		// Save the current settings to a new LUT file
		if (ImGui::Button("Save As File"))
		{
			ImGui::OpenPopup("Save LUT As");
		}

		// Spawn Save As Popup
		if (ImGui::BeginPopupModal("Save LUT As", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			static char lutName[128] = "NewColorGradeLUT";
			ImGui::InputText("LUT Name", lutName, sizeof(lutName));

			ImGui::Spacing();

			if (ImGui::Button("Create", ImVec2(120, 0)))
			{
				auto texDir = ProjectManager::GetActive()->GetAssetDirectory() / "Textures";
				std::string newTexPath = (texDir / std::format("{}.png", lutName)).string();
				if (std::filesystem::exists(std::filesystem::absolute(newTexPath)))
				{
					ImGui::OpenPopup("File Exists");
				}
				else
				{
					// Bake the lut and save to disk and as an asset
					auto renderSystem = Application::Instance().GetSystem<RenderSystem>();
					renderSystem->BakeColorGradeLUT(colorGradeProps, newTexPath);
					auto newLUTTex = Application::Instance().GetAssetManager().Load<Texture2D>(UUID(), lutName, newTexPath, false);

					// Reset buffer and close
					strcpy(lutName, "NewColorGradeLUT");
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
			}

			// Nested Warning Popup
			if (ImGui::BeginPopupModal("File Exists", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("A script with that name already exists.\nPlease choose a different name.");
				ImGui::Spacing();
				if (ImGui::Button("OK", ImVec2(120, 0)))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::SameLine();

		// Reset to default settings
		if (ImGui::Button("Reset to Default"))
		{
			colorGradePass->Settings.Reset();
			toneMapPass->Exposure = 1.0f;
		}
	}

	void EnvironmentPanel::RenderColorGradeSettings()
	{
		auto renderSystem = Application::Instance().GetSystem<RenderSystem>();
		auto colorGradePass = StaticPointerCast<ColorGradePass>(renderSystem->GetPostProcessPass("ColorGradePass"));
		if (UI::Nodes::BeginEnabledExpandableNode("Color Grading", colorGradePass->Enabled))
		{
			auto toneMapPass = StaticPointerCast<ToneMapPass>(renderSystem->GetPostProcessPass("ToneMapPass"));
			auto& colorGradeProps = colorGradePass->Settings;

			ImGui::BeginDisabled(!colorGradePass->Enabled);
			RenderColorGradeLUTSettings(colorGradePass, toneMapPass);

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