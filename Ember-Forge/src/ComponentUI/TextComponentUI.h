#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"
#include "UI/DragDropTypes.h"

#include "Ember/Asset/Font.h"

#include <imgui/imgui.h>

namespace Ember {

	class TextComponentUI : public ComponentUI<TextComponent>
	{
	public:
		TextComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Text Component"; }

	protected:
		inline void RenderComponentImpl(TextComponent& component) override
		{
			if (UI::PropertyGrid::Begin("TextProps"))
			{
				UI::PropertyGrid::InputText("Text", component.Text);
				UI::PropertyGrid::Color4("Color", component.Color);

				// Font asset selector
				auto& assetManager = Application::Instance().GetAssetManager();

				auto chooseFontFunc = [&]() {
					ImGui::OpenPopup("ChooseFontPopup");
					};
				auto clearFontFunc = UI::UICallbackFunc([&]() {
					component.FontHandle = Constants::InvalidUUID;
					});

				std::string selectedFont;
				bool fontExists = component.FontHandle != Constants::InvalidUUID;
				if (fontExists)
				{
					auto fontAsset = assetManager.GetAsset<Font>(component.FontHandle);
					if (fontAsset)
					{
						UI::PropertyGrid::AssetReference("Font", fontAsset->GetName(), DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetFont),
							selectedFont, chooseFontFunc, clearFontFunc);
					}
				}
				else
				{
					UI::PropertyGrid::AssetReference("Font", "None (Font)", DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetFont),
						selectedFont, chooseFontFunc, nullptr);
				}

				// Set component handle
				if (!selectedFont.empty())
				{
					auto fontAsset = assetManager.GetAssetByPath<Font>(selectedFont);
					if (fontAsset != nullptr)
						component.FontHandle = fontAsset->GetUUID();
					else
					{
						auto fontAsset = assetManager.Load<Font>(selectedFont);
						component.FontHandle = fontAsset ? fontAsset->GetUUID() : (UUID)Constants::InvalidUUID;
					}
				}

				if (ImGui::BeginPopup("ChooseFontPopup"))
				{
					if (ImGui::MenuItem("Load from file..."))
					{
						std::string fontFile = FileDialog::OpenFile("Ember-Forge/assets/fonts", "Font (*.ttf;*.otf;*.ebfont)", "*.ttf;*.otf;*.ebfont");
						if (!fontFile.empty())
						{
							auto fontAsset = assetManager.Load<Font>(fontFile);
							if (fontAsset)
								component.FontHandle = fontAsset->GetUUID();
						}
					}

					ImGui::EndPopup();
				}
				
				UI::PropertyGrid::End();
			}
		}
	};

}