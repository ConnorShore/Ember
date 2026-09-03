#pragma once

#include "UI/PropertyGrid.h"
#include "UI/DragDropTypes.h"

#include <Ember/Asset/ParticleEffect.h>
#include <Ember/Asset/AssetManager.h>
#include <Ember/Core/ProjectManager.h>
#include <Ember/Render/Texture2D.h>
#include <Ember/Utils/PlatformUtil.h>

#include <imgui/imgui.h>

#include <filesystem>
#include <format>
#include <string>

namespace Ember::UI {

	// Shared by the inspector and the particle effect editor tab so both panes stay in step.
	inline bool DrawParticleEffectProperties(ParticleEffectProperties& properties, AssetManager& assetManager, const char* idScope)
	{
		bool changed = false;

		ImGui::PushID(idScope);

		if (ImGui::TreeNodeEx("General", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (UI::PropertyGrid::Begin("ParticleGeneralProps"))
			{
				changed |= UI::PropertyGrid::Float("Emission Rate", properties.EmissionRate);
				changed |= UI::PropertyGrid::Float("Lifetime", properties.Lifetime);
				changed |= UI::PropertyGrid::Float("Lifetime Variation", properties.LifetimeVariation);

				UI::PropertyGrid::End();
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Physics & Movement"))
		{
			if (UI::PropertyGrid::Begin("ParticlePhysicsProps"))
			{
				changed |= UI::PropertyGrid::Float3("Velocity", properties.Velocity);
				changed |= UI::PropertyGrid::Float3("Velocity Variation", properties.VelocityVariation);
				changed |= UI::PropertyGrid::Float("Gravity Multiplier", properties.GravityMultiplier);
				changed |= UI::PropertyGrid::Float("Drag", properties.Drag);

				UI::PropertyGrid::End();
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Rotation & Alignment"))
		{
			if (UI::PropertyGrid::Begin("ParticleRotationProps"))
			{
				changed |= UI::PropertyGrid::Checkbox("Align With Velocity", properties.AlignWithVelocity);

				// UX Trick: Hide/Show properties based on the alignment mode!
				if (properties.AlignWithVelocity)
				{
					changed |= UI::PropertyGrid::Float("Stretch Factor", properties.StretchFactor, 0.01f);
				}
				else
				{
					changed |= UI::PropertyGrid::Float("Rotation Speed", properties.AngularVelocity);
					changed |= UI::PropertyGrid::Float("Rotation Speed Variation", properties.AngularVelocityVariation);
				}

				UI::PropertyGrid::End();
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Visuals", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (UI::PropertyGrid::Begin("ParticleVisualsProps"))
			{
				changed |= UI::PropertyGrid::Color4("Color Begin", properties.ColorBegin);
				changed |= UI::PropertyGrid::Color4("Color End", properties.ColorEnd);
				changed |= UI::PropertyGrid::Float("Color Intensity", properties.ColorIntensity);

				if (UI::PropertyGrid::BeginComboBox("Blend Mode", ParticleBlendModeToString(properties.BlendMode)))
				{
					if (UI::PropertyGrid::ComboBoxItem("Alpha", properties.BlendMode == ParticleBlendMode::Alpha))
					{
						properties.BlendMode = ParticleBlendMode::Alpha;
						changed = true;
					}
					if (UI::PropertyGrid::ComboBoxItem("Additive", properties.BlendMode == ParticleBlendMode::Additive))
					{
						properties.BlendMode = ParticleBlendMode::Additive;
						changed = true;
					}
					UI::PropertyGrid::EndComboBox();
				}

				changed |= UI::PropertyGrid::Float("Scale Begin", properties.ScaleBegin);
				changed |= UI::PropertyGrid::Float("Scale End", properties.ScaleEnd);
				changed |= UI::PropertyGrid::Float("Scale Variation", properties.ScaleVariation);

				bool textureExists = properties.TextureHandle != Constants::Assets::DefaultWhiteTexUUID;
				std::string fileName = "None (Texture)";

				if (textureExists)
				{
					auto textureAsset = assetManager.GetAsset<Texture>(properties.TextureHandle);
					if (textureAsset)
						fileName = std::filesystem::path(textureAsset->GetFilePath()).filename().string();
				}

				auto textureDir = ProjectManager::GetActive()->GetDefaultDirectoryForAsset(AssetType::Texture);
				std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetTexture);
				std::string droppedPath;

				UI::UICallbackFunc browseFunc = [&]() {
					std::string extensions = DragDropUtils::DragDropPayloadTypeToExtension(DragDropPayloadType::AssetTexture);
					std::string textureFile = FileDialog::OpenFile(textureDir.string().c_str(), std::format("Textures ({})", extensions).c_str(), extensions.c_str());
					if (!textureFile.empty())
					{
						auto texture = assetManager.Load<Texture>(textureFile);
						if (texture)
						{
							properties.TextureHandle = texture->GetUUID();
							changed = true;
						}
					}
					};

				UI::UICallbackFunc clearFunc = textureExists ? UI::UICallbackFunc([&]() {
					properties.TextureHandle = Constants::Assets::DefaultWhiteTexUUID;
					changed = true;
					}) : nullptr;

				if (UI::PropertyGrid::AssetReference("Texture Asset", fileName, payloadType, droppedPath, browseFunc, clearFunc))
				{
					auto texture = assetManager.Load<Texture>(droppedPath);
					if (texture)
					{
						properties.TextureHandle = texture->GetUUID();
						changed = true;
					}
				}

				UI::PropertyGrid::End();
			}
			ImGui::TreePop();
		}

		ImGui::PopID();

		return changed;
	}

}
