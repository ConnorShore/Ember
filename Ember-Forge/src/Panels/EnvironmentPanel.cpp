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
		// Enable bloom and color grade by default
		m_PostProcessVolumeSettings.BloomEnabled = true;
		m_PostProcessVolumeSettings.ColorGradeEnabled = true;

	}

	void EnvironmentPanel::OnAttach()
	{
		// Populate m_PostProcessVolumeSettings with default values from the render system's passes
		auto renderSystem = Application::Instance().GetSystem<RenderSystem>();
		auto bloomPass = StaticPointerCast<BloomPass>(renderSystem->GetPostProcessPass("BloomPass"));
		auto colorGradingPass = StaticPointerCast<ColorGradePass>(renderSystem->GetPostProcessPass("ColorGradePass"));
		auto toneMapPass = StaticPointerCast<ToneMapPass>(renderSystem->GetPostProcessPass("ToneMapPass"));
		auto fogPass = StaticPointerCast<FogPass>(renderSystem->GetPostProcessPass("FogPass"));
		auto vignettePass = StaticPointerCast<VignettePass>(renderSystem->GetPostProcessPass("VignettePass"));

		m_PostProcessVolumeSettings.Bloom = bloomPass->Settings;
		m_PostProcessVolumeSettings.ColorGrade = colorGradingPass->Settings;
		m_PostProcessVolumeSettings.ToneMap = toneMapPass->Settings;
		m_PostProcessVolumeSettings.Fog = fogPass->Settings;
		m_PostProcessVolumeSettings.Vignette = vignettePass->Settings;
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

		// Update global post process volume settings in the render system
		auto renderSystem = Application::Instance().GetSystem<RenderSystem>();
		renderSystem->SetGlobalPostProcessVolumeSettings(m_PostProcessVolumeSettings);
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
		if (UI::Nodes::BeginEnabledExpandableNode("Bloom", m_PostProcessVolumeSettings.BloomEnabled))
		{
			if (UI::PropertyGrid::Begin("##BloomPropertyGrid"))
			{
				ImGui::BeginDisabled(!m_PostProcessVolumeSettings.BloomEnabled);

				UI::PropertyGrid::Float("Threshold", m_PostProcessVolumeSettings.Bloom.Threshold, 0.01f, 0.0f, 10.0f);
				UI::PropertyGrid::Float("Soft Knee", m_PostProcessVolumeSettings.Bloom.Knee, 0.01f, 0.0f, 1.0f);
				UI::PropertyGrid::Float("Intensity", m_PostProcessVolumeSettings.Bloom.Intensity, 0.01f, 0.0f, 5.0f);
				UI::PropertyGrid::Float("Blur Radius", m_PostProcessVolumeSettings.Bloom.BlurRadius, 0.01f, 0.1f, 5.0f);

				ImGui::EndDisabled();

				UI::PropertyGrid::End();
			}

			UI::Nodes::EndExpandableNode();
		}
	}

	void EnvironmentPanel::RenderFogSettings()
	{
		if (UI::Nodes::BeginEnabledExpandableNode("Fog", m_PostProcessVolumeSettings.FogEnabled))
		{
			if (UI::PropertyGrid::Begin("##FogPropertyGrid"))
			{
				ImGui::BeginDisabled(!m_PostProcessVolumeSettings.FogEnabled);
				UI::PropertyGrid::Color3("Color", m_PostProcessVolumeSettings.Fog.Color);
				UI::PropertyGrid::Float("Density", m_PostProcessVolumeSettings.Fog.Density, 0.001f, 0.0f, 1.0f);
				UI::PropertyGrid::Float("Start Distance", m_PostProcessVolumeSettings.Fog.StartDistance, 0.01f, 0.0f, 100000.0f);
				UI::PropertyGrid::Float("Falloff", m_PostProcessVolumeSettings.Fog.Falloff, 0.01f, 0.0f, 100000.0f);
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
		if (UI::Nodes::BeginEnabledExpandableNode("Vignette", m_PostProcessVolumeSettings.VignetteEnabled))
		{
			if (UI::PropertyGrid::Begin("##VignettePropertyGrid"))
			{
				ImGui::BeginDisabled(!m_PostProcessVolumeSettings.VignetteEnabled);
				UI::PropertyGrid::Color3("Color", m_PostProcessVolumeSettings.Vignette.Color);
				UI::PropertyGrid::Float("Intensity", m_PostProcessVolumeSettings.Vignette.Intensity, 0.01f, 0.0f, 5.0f);
				UI::PropertyGrid::Float("Size", m_PostProcessVolumeSettings.Vignette.Size, 0.001f, 0.0f, 1.0f);
				UI::PropertyGrid::Float("Smoothness", m_PostProcessVolumeSettings.Vignette.Smoothness, 0.001f, 0.0f, 1.0f);
				ImGui::EndDisabled();

				UI::PropertyGrid::End();
			}
			UI::Nodes::EndExpandableNode();
		}
	}

	void EnvironmentPanel::RenderColorGradeLUTSettings(const SharedPtr<ColorGradePass>& colorGradePass, const SharedPtr<ToneMapPass>& toneMapPass)
	{
		// TOOD: Set color grade lut to ColorGradeSettings
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
			toneMapPass->Settings.Exposure = 1.0f;
		}
	}

	void EnvironmentPanel::RenderColorGradeSettings()
	{
		auto renderSystem = Application::Instance().GetSystem<RenderSystem>();
		auto colorGradePass = StaticPointerCast<ColorGradePass>(renderSystem->GetPostProcessPass("ColorGradePass"));
		if (UI::Nodes::BeginEnabledExpandableNode("Color Grading", m_PostProcessVolumeSettings.ColorGradeEnabled))
		{
			auto toneMapPass = StaticPointerCast<ToneMapPass>(renderSystem->GetPostProcessPass("ToneMapPass"));
			auto& colorGradeProps = colorGradePass->Settings;

			ImGui::BeginDisabled(!m_PostProcessVolumeSettings.ColorGradeEnabled);
			RenderColorGradeLUTSettings(colorGradePass, toneMapPass);

			if (ImGui::TreeNode("Exposure"))
			{
				if (UI::PropertyGrid::Begin("##ExposureProps"))
				{
					UI::PropertyGrid::Float("Exposure", m_PostProcessVolumeSettings.ToneMap.Exposure, 0.01f, 0.0f, 10.0f);
					UI::PropertyGrid::End();
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("White Balance"))
			{
				if (UI::PropertyGrid::Begin("##WhiteBalanceProps"))
				{
					UI::PropertyGrid::Float("Temperature", m_PostProcessVolumeSettings.ColorGrade.Temperature, 0.01f, -1.0f, 1.0f);
					UI::PropertyGrid::Float("Tint", m_PostProcessVolumeSettings.ColorGrade.Tint, 0.01f, -1.0f, 1.0f);
					UI::PropertyGrid::End();
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Color Adjustments"))
			{
				if (UI::PropertyGrid::Begin("##ColorAdjustmentProps"))
				{
					UI::PropertyGrid::Float("Contrast", m_PostProcessVolumeSettings.ColorGrade.Contrast, 0.01f, 0.0f, 2.0f);
					UI::PropertyGrid::Float("Saturation", m_PostProcessVolumeSettings.ColorGrade.Saturation, 0.01f, 0.0f, 2.0f);

					UI::PropertyGrid::End();
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Lift, Gamma, Gain"))
			{
				if (UI::PropertyGrid::Begin("##LGGProps"))
				{
					UI::PropertyGrid::Color4("Lift", m_PostProcessVolumeSettings.ColorGrade.Lift);
					UI::PropertyGrid::Color4("Gamma", m_PostProcessVolumeSettings.ColorGrade.Gamma);
					UI::PropertyGrid::Color4("Gain", m_PostProcessVolumeSettings.ColorGrade.Gain);

					UI::PropertyGrid::End();
				}
				ImGui::TreePop();
			}
			ImGui::EndDisabled();

			UI::Nodes::EndExpandableNode();
		}
	}

}