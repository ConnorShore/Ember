#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"
#include "UI/DragDropTypes.h"

#include <Ember/Asset/Font.h>

#include <Ember/Render/VFX/BloomPass.h>
#include <Ember/Render/VFX/FogPass.h>
#include <Ember/Render/VFX/ColorGradePass.h>
#include <Ember/Render/VFX/ToneMapPass.h>
#include <Ember/Render/VFX/VignettePass.h>

#include <Ember/ECS/System/RenderSystem.h>

#include <imgui/imgui.h>

namespace Ember {

	class PostProcessVolumeComponentUI : public ComponentUI<PostProcessVolumeComponent>
	{
	public:
		PostProcessVolumeComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Post Process Volume Component"; }

	protected:
		inline void RenderComponentImpl(PostProcessVolumeComponent& component) override
		{
			if (UI::PropertyGrid::Begin("##PostProcessVolumePreview"))
			{
				UI::PropertyGrid::Checkbox("Preview", m_Preview);
				UI::PropertyGrid::End();
			}

			ImGui::Separator();

			RenderBloomSettings(component);
			RenderFogSettings(component);
			RenderVignetteSettings(component);
			RenderColorGradeSettings(component);

			if (m_Preview)
			{
				auto renderSystem = Application::Instance().GetSystem<RenderSystem>();
				renderSystem->SetGlobalPostProcessVolumeSettings(component.Settings);
			}
		}

	private:
		void RenderBloomSettings(PostProcessVolumeComponent& component)
		{
			if (UI::Nodes::BeginEnabledExpandableNode("Bloom", component.Settings.BloomEnabled))
			{
				if (UI::PropertyGrid::Begin("##BloomPropertyGrid"))
				{
					ImGui::BeginDisabled(!component.Settings.BloomEnabled);

					UI::PropertyGrid::Float("Threshold", component.Settings.Bloom.Threshold, 0.01f, 0.0f, 10.0f);
					UI::PropertyGrid::Float("Soft Knee", component.Settings.Bloom.Knee, 0.01f, 0.0f, 1.0f);
					UI::PropertyGrid::Float("Intensity", component.Settings.Bloom.Intensity, 0.01f, 0.0f, 5.0f);
					UI::PropertyGrid::Float("Blur Radius", component.Settings.Bloom.BlurRadius, 0.01f, 0.1f, 5.0f);

					ImGui::EndDisabled();

					UI::PropertyGrid::End();
				}

				UI::Nodes::EndExpandableNode();
			}
		}

		void RenderFogSettings(PostProcessVolumeComponent& component)
		{
			if (UI::Nodes::BeginEnabledExpandableNode("Fog", component.Settings.FogEnabled))
			{
				if (UI::PropertyGrid::Begin("##FogPropertyGrid"))
				{
					ImGui::BeginDisabled(!component.Settings.FogEnabled);
					UI::PropertyGrid::Color3("Color", component.Settings.Fog.Color);
					UI::PropertyGrid::Float("Density", component.Settings.Fog.Density, 0.001f, 0.0f, 1.0f);
					UI::PropertyGrid::Float("Start Distance", component.Settings.Fog.StartDistance, 0.01f, 0.0f, 100000.0f);
					UI::PropertyGrid::Float("Falloff", component.Settings.Fog.Falloff, 0.01f, 0.0f, 100000.0f);
					ImGui::EndDisabled();

					UI::PropertyGrid::End();
				}
				UI::Nodes::EndExpandableNode();
			}
		}

		void RenderVignetteSettings(PostProcessVolumeComponent& component)
		{
			if (UI::Nodes::BeginEnabledExpandableNode("Vignette", component.Settings.VignetteEnabled))
			{
				if (UI::PropertyGrid::Begin("##VignettePropertyGrid"))
				{
					ImGui::BeginDisabled(!component.Settings.VignetteEnabled);
					UI::PropertyGrid::Color3("Color", component.Settings.Vignette.Color);
					UI::PropertyGrid::Float("Intensity", component.Settings.Vignette.Intensity, 0.01f, 0.0f, 5.0f);
					UI::PropertyGrid::Float("Size", component.Settings.Vignette.Size, 0.001f, 0.0f, 1.0f);
					UI::PropertyGrid::Float("Smoothness", component.Settings.Vignette.Smoothness, 0.001f, 0.0f, 1.0f);
					ImGui::EndDisabled();

					UI::PropertyGrid::End();
				}
				UI::Nodes::EndExpandableNode();
			}
		}

		void RenderColorGradeSettings(PostProcessVolumeComponent& component)
		{
			if (UI::Nodes::BeginEnabledExpandableNode("Color Grading", component.Settings.ColorGradeEnabled))
			{
				auto& colorGradeProps = component.Settings.ColorGrade;

				ImGui::BeginDisabled(!component.Settings.ColorGradeEnabled);
				//RenderColorGradeLUTSettings(colorGradePass, toneMapPass);

				if (ImGui::TreeNode("Exposure"))
				{
					if (UI::PropertyGrid::Begin("##ExposureProps"))
					{
						UI::PropertyGrid::Float("Exposure", component.Settings.ToneMap.Exposure, 0.01f, 0.0f, 10.0f);
						UI::PropertyGrid::End();
					}
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("White Balance"))
				{
					if (UI::PropertyGrid::Begin("##WhiteBalanceProps"))
					{
						UI::PropertyGrid::Float("Temperature", component.Settings.ColorGrade.Temperature, 0.01f, -1.0f, 1.0f);
						UI::PropertyGrid::Float("Tint", component.Settings.ColorGrade.Tint, 0.01f, -1.0f, 1.0f);
						UI::PropertyGrid::End();
					}
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("Color Adjustments"))
				{
					if (UI::PropertyGrid::Begin("##ColorAdjustmentProps"))
					{
						UI::PropertyGrid::Float("Contrast", component.Settings.ColorGrade.Contrast, 0.01f, 0.0f, 2.0f);
						UI::PropertyGrid::Float("Saturation", component.Settings.ColorGrade.Saturation, 0.01f, 0.0f, 2.0f);

						UI::PropertyGrid::End();
					}
					ImGui::TreePop();
				}

				if (ImGui::TreeNode("Lift, Gamma, Gain"))
				{
					if (UI::PropertyGrid::Begin("##LGGProps"))
					{
						UI::PropertyGrid::Color4("Lift", component.Settings.ColorGrade.Lift);
						UI::PropertyGrid::Color4("Gamma", component.Settings.ColorGrade.Gamma);
						UI::PropertyGrid::Color4("Gain", component.Settings.ColorGrade.Gain);

						UI::PropertyGrid::End();
					}
					ImGui::TreePop();
				}
				ImGui::EndDisabled();

				UI::Nodes::EndExpandableNode();
			}
		}
	private:
		bool m_Preview = false;
	};

}