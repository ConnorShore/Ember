#pragma once
#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"
#include "UI/DragDropTypes.h"

#include "Ember/Asset/Font.h"

#include <imgui/imgui.h>

namespace Ember {

	class SpriteComponentUI : public ComponentUI<SpriteComponent>
	{
	public:
		SpriteComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Sprite Component"; }

	protected:
		inline void RenderComponentImpl(SpriteComponent& component) override
		{
			if (UI::PropertyGrid::Begin("SpriteProps"))
			{
				UI::PropertyGrid::Color4("Color", component.Color);

				// Texture asset selector
				SharedPtr<Texture2D> currentTexture = nullptr;
				bool hasTexture = component.TextureHandle != Constants::InvalidUUID;
				if (hasTexture)
					currentTexture = Application::Instance().GetAssetManager().GetAsset<Texture2D>(component.TextureHandle);

				bool hasValidTexture = currentTexture
					&& currentTexture->GetName() != Constants::Assets::DefaultWhiteTex
					&& currentTexture->GetName() != Constants::Assets::DefaultNormalTex
					&& currentTexture->GetName() != Constants::Assets::DefaultErrorTex;
				UUID texUUID = hasValidTexture ? currentTexture->GetUUID() : UUID(Constants::InvalidUUID);
				std::string droppedFilePath;
				if (UI::PropertyGrid::DragDropTexture("Image", texUUID, droppedFilePath, [&]() {
					component.TextureHandle = Constants::InvalidUUID;
				}))
				{
					auto newTexture = Application::Instance().GetAssetManager().Load<Texture2D>(droppedFilePath);
					component.TextureHandle = newTexture->GetUUID();
				}

				UI::PropertyGrid::End();
			}
		}
	};

}