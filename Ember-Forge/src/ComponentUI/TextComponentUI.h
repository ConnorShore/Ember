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
				UI::PropertyGrid::Float("Font Size", component.FontSize, 1.0f, 1.0f, 512.0f);

				static const char* alignmentNames[] = { "Start", "Center", "End" };
				if (UI::PropertyGrid::BeginComboBox("Horizontal Align", alignmentNames[(int)component.HorizontalAlignment]))
				{
					for (int i = 0; i < IM_ARRAYSIZE(alignmentNames); i++)
					{
						if (UI::PropertyGrid::ComboBoxItem(alignmentNames[i], (int)component.HorizontalAlignment == i))
							component.HorizontalAlignment = (TextAlignment)i;
					}
					UI::PropertyGrid::EndComboBox();
				}

				if (UI::PropertyGrid::BeginComboBox("Vertical Align", alignmentNames[(int)component.VerticalAlignment]))
				{
					for (int i = 0; i < IM_ARRAYSIZE(alignmentNames); i++)
					{
						if (UI::PropertyGrid::ComboBoxItem(alignmentNames[i], (int)component.VerticalAlignment == i))
							component.VerticalAlignment = (TextAlignment)i;
					}
					UI::PropertyGrid::EndComboBox();
				}

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
					auto fontAsset = m_AssetManager.GetAsset<Font>(component.FontHandle);
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
					auto fontAsset = m_AssetManager.GetAssetByPath<Font>(selectedFont);
					if (fontAsset != nullptr)
						component.FontHandle = fontAsset->GetUUID();
					else
					{
						auto fontAsset = m_AssetManager.Load<Font>(selectedFont);
						component.FontHandle = fontAsset ? fontAsset->GetUUID() : (UUID)Constants::InvalidUUID;
					}
				}

				if (ImGui::BeginPopup("ChooseFontPopup"))
				{
					if (ImGui::MenuItem("Load from file..."))
					{
						std::string defaultDir = ProjectManager::GetActive()->GetDefaultDirectoryForAsset(AssetType::Font).string();
						std::string fontFile = FileDialog::OpenFile(defaultDir.c_str(), "Font (*.ttf;*.otf;*.ebfont)", "*.ttf;*.otf;*.ebfont");
						if (!fontFile.empty())
						{
							auto fontAsset = m_AssetManager.Load<Font>(fontFile);
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