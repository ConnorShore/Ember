#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"

namespace Ember {

	class ParticleEmitterComponentUI : public ComponentUI<ParticleEmitterComponent>
	{
	public:
		ParticleEmitterComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Particle Emitter Component"; }

	protected:
		inline void RenderComponentImpl(ParticleEmitterComponent& component) override
		{
			if (UI::PropertyGrid::Begin("ParticleEmitterProps"))
			{
				UI::PropertyGrid::Checkbox("Active", component.IsActive);

				UI::PropertyGrid::Float("Emission Rate", component.EmissionRate);
				UI::PropertyGrid::Float("Gravity Multiplier", component.GravityMultiplier);

				UI::PropertyGrid::Float3("Velocity", component.Velocity);
				UI::PropertyGrid::Float3("Velocity Variation", component.VelocityVariation);

				UI::PropertyGrid::Color4("Color Begin", component.ColorBegin);
				UI::PropertyGrid::Color4("Color End", component.ColorEnd);

				UI::PropertyGrid::Float("Scale Begin", component.ScaleBegin);
				UI::PropertyGrid::Float("Scale End", component.ScaleEnd);
				UI::PropertyGrid::Float("Scale Variation", component.ScaleVariation);

				UI::PropertyGrid::Float("Lifetime", component.Lifetime);
				UI::PropertyGrid::Float("Lifetime Variation", component.LifetimeVariation);

				// Texture asset picker
				// TODO: Make this all more generic. Alot of these pickers are the same  and just need a different
				// dropdown menu.  Maybe just pass in the asset type and a dropdown function or something
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
					std::string textureFile = FileDialog::OpenFile(textureDir.string().c_str(), "Textures (*.png)", "*.png");
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
		}
	};

}