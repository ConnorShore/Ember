#pragma once
#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"

#include <imgui/imgui.h>

namespace Ember {

	class ParticleEmitterComponentUI : public ComponentUI<ParticleEmitterComponent>
	{
	public:
		ParticleEmitterComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Particle Emitter Component"; }

	protected:
		inline void RenderComponentImpl(ParticleEmitterComponent& component) override
		{
			// Push the component's memory address as an ID so multiple emitters don't clash in ImGui
			ImGui::PushID(&component);

			// --- 1. GENERAL SECTION ---
			// Using DefaultOpen so the user immediately sees the most critical properties
			if (ImGui::TreeNodeEx("General", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (UI::PropertyGrid::Begin("ParticleGeneralProps"))
				{
					UI::PropertyGrid::Checkbox("Active", component.IsActive);
					UI::PropertyGrid::Float("Emission Rate", component.EmissionRate);
					UI::PropertyGrid::Float("Lifetime", component.Lifetime);
					UI::PropertyGrid::Float("Lifetime Variation", component.LifetimeVariation);

					UI::PropertyGrid::End();
				}
				ImGui::TreePop();
			}

			// --- 2. PHYSICS & MOVEMENT SECTION ---
			if (ImGui::TreeNode("Physics & Movement"))
			{
				if (UI::PropertyGrid::Begin("ParticlePhysicsProps"))
				{
					UI::PropertyGrid::Float3("Velocity", component.Velocity);
					UI::PropertyGrid::Float3("Velocity Variation", component.VelocityVariation);
					UI::PropertyGrid::Float("Gravity Multiplier", component.GravityMultiplier);
					UI::PropertyGrid::Float("Drag", component.Drag);

					UI::PropertyGrid::End();
				}
				ImGui::TreePop();
			}

			// --- 3. ROTATION & ALIGNMENT SECTION ---
			if (ImGui::TreeNode("Rotation & Alignment"))
			{
				if (UI::PropertyGrid::Begin("ParticleRotationProps"))
				{
					UI::PropertyGrid::Checkbox("Align With Velocity", component.AlignWithVelocity);

					// UX Trick: Hide/Show properties based on the alignment mode!
					if (component.AlignWithVelocity)
					{
						UI::PropertyGrid::Float("Stretch Factor", component.StretchFactor, 0.01f);
					}
					else
					{
						UI::PropertyGrid::Float("Rotation Speed", component.AngularVelocity);
						UI::PropertyGrid::Float("Rotation Speed Variation", component.AngularVelocityVariation);
					}

					UI::PropertyGrid::End();
				}
				ImGui::TreePop();
			}

			// --- 4. VISUALS SECTION ---
			if (ImGui::TreeNode("Visuals"))
			{
				if (UI::PropertyGrid::Begin("ParticleVisualsProps"))
				{
					UI::PropertyGrid::Color4("Color Begin", component.ColorBegin);
					UI::PropertyGrid::Color4("Color End", component.ColorEnd);

					UI::PropertyGrid::Float("Scale Begin", component.ScaleBegin);
					UI::PropertyGrid::Float("Scale End", component.ScaleEnd);
					UI::PropertyGrid::Float("Scale Variation", component.ScaleVariation);

					// Texture asset picker
					auto& assetManager = Application::Instance().GetAssetManager();
					bool textureExists = component.TextureHandle != Constants::Assets::DefaultWhiteTexUUID;
					std::string fileName = "None (Texture)";

					if (textureExists)
					{
						auto textureAsset = assetManager.GetAsset<Texture>(component.TextureHandle);
						if (textureAsset)
							fileName = std::filesystem::path(textureAsset->GetFilePath()).filename().string();
					}

					auto textureDir = ProjectManager::GetActive()->GetAssetDirectory() / "Textures";
					std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetTexture);
					std::string droppedPath;

					UI::UICallbackFunc browseFunc = [&]() {
						std::string extensions = DragDropUtils::DragDropPayloadTypeToExtension(DragDropPayloadType::AssetTexture);
						std::string textureFile = FileDialog::OpenFile(textureDir.string().c_str(), std::format("Textures ({})", extensions).c_str(), extensions.c_str());
						if (!textureFile.empty())
						{
							auto texture = assetManager.Load<Texture>(textureFile);
							component.TextureHandle = texture->GetUUID();
						}
						};

					UI::UICallbackFunc clearFunc = textureExists ? UI::UICallbackFunc([&]() {
						component.TextureHandle = Constants::Assets::DefaultWhiteTexUUID;
						}) : nullptr;

					if (UI::PropertyGrid::AssetReference("Texture Asset", fileName, payloadType, droppedPath, browseFunc, clearFunc))
					{
						auto texture = assetManager.Load<Texture>(droppedPath);
						component.TextureHandle = texture->GetUUID();
					}

					UI::PropertyGrid::End();
				}
				ImGui::TreePop();
			}

			ImGui::PopID();
		}
	};

}