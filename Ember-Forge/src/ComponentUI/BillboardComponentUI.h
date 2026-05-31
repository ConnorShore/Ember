#pragma once
#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"
#include "UI/DragDropTypes.h"

#include "Ember/Asset/Font.h"

#include <imgui/imgui.h>

namespace Ember {

	class EditorIconComponentUI : public ComponentUI<EditorIconComponent>
	{
	public:
		EditorIconComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Editor Icon Component"; }

		virtual void CreateComponentForEntity(Entity entity) override
		{
			entity.AttachComponent<EditorIconComponent>();
		}

	protected:
		inline void RenderComponentImpl(EditorIconComponent& component) override
		{
			if (UI::PropertyGrid::Begin("BillboardProps"))
			{
				UI::PropertyGrid::Color4("Color", component.Tint);
				UI::PropertyGrid::Checkbox("Spherical", component.Spherical);
				UI::PropertyGrid::Checkbox("Static Size", component.StaticSize);

				// Texture asset selector
				SharedPtr<Texture2D> currentTexture = nullptr;
				bool hasTexture = component.TextureHandle != Constants::InvalidUUID;
				if (hasTexture)
					currentTexture = m_AssetManager.GetAsset<Texture2D>(component.TextureHandle);

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
					auto newTexture = m_AssetManager.Load<Texture2D>(droppedFilePath);
					component.TextureHandle = newTexture->GetUUID();
				}

				UI::PropertyGrid::End();
			}
		}
	};

}