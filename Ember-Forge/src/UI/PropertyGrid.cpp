#include "efpch.h"

#include "Types.h"
#include "UIWidgets.h"
#include "PropertyGrid.h"
#include "DragDropTypes.h"

#include <Ember/Core/Application.h>

namespace Ember {
	namespace UI::PropertyGrid {

		// Property Grid Layout
		bool Begin(const std::string& id)
		{
			ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable;
			if (ImGui::BeginTable(id.c_str(), 2, flags))
			{
				ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
				return true;
			}
			return false;
		}

		void End()
		{
			ImGui::EndTable();
		}

		// Property Grid Items
		bool HeaderWithActionButton(const std::string& headerLabel, const std::string& buttonLabel, const std::string& caption /* = "" */)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(headerLabel.c_str());

			// Actions
			ImGui::TableNextColumn();

			float buttonWidth = ImGui::CalcTextSize(buttonLabel.c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f;
			float totalWidthRequired = buttonWidth;

			std::string formattedCaption = "";
			if (!caption.empty())
			{
				formattedCaption = "(" + caption + ")";
				totalWidthRequired += ImGui::CalcTextSize(formattedCaption.c_str()).x;
				totalWidthRequired += ImGui::GetStyle().ItemSpacing.x;
			}

			float posX = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - totalWidthRequired;
			ImGui::SetCursorPosX(posX);
			
			if (!caption.empty())
			{
				ImGui::AlignTextToFramePadding();
				ImGui::TextDisabled("%s", formattedCaption.c_str());
				ImGui::SameLine();
			}

			return ImGui::Button(buttonLabel.c_str());
		}

		bool Checkbox(const std::string& label, bool& value)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(label.c_str());
			ImGui::TableNextColumn();
			ImGui::PushItemWidth(-FLT_MIN);
			return ImGui::Checkbox(std::format("##{}", label).c_str(), &value);
		}
		bool InputText(const std::string& label, std::string& value)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", label.c_str());

			ImGui::TableNextColumn();
			ImGui::PushItemWidth(-FLT_MIN);

			char buffer[256];
			strncpy_s(buffer, sizeof(buffer), value.c_str(), _TRUNCATE);

			bool modified = false;
			if (ImGui::InputText(std::format("##{}", label).c_str(), buffer, sizeof(buffer)))
			{
				value = std::string(buffer);
				modified = true;
			}
			ImGui::PopItemWidth();
			return modified;
		}

		bool DirectoryInput(const std::string& label, std::string& directoryPath, UICallbackFunc browseFunc)
		{
			ImGui::PushID(label.c_str());
			bool modified = false;

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", label.c_str());

			ImGui::TableNextColumn();

			float buttonSize = ImGui::GetFrameHeight();
			float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
			float inputWidth = ImGui::GetContentRegionAvail().x - buttonSize - spacing;

			ImGui::PushItemWidth(inputWidth);
			char buffer[512];
			strncpy_s(buffer, sizeof(buffer), directoryPath.c_str(), _TRUNCATE);
			if (ImGui::InputText("##Path", buffer, sizeof(buffer)))
			{
				directoryPath = std::string(buffer);
				modified = true;
			}
			ImGui::PopItemWidth();

			if (browseFunc)
			{
				ImGui::SameLine(0, spacing);
				if (ImGui::Button("...", ImVec2(buttonSize, buttonSize)))
				{
					browseFunc();
					modified = true;
				}
			}

			ImGui::PopID();
			return modified;
		}

		// Property Grid Widgets

		bool SliderInt(const std::string& label, int& value, int min /* = 0 */, int max /* = 0 */)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(label.c_str());
			ImGui::TableNextColumn();
			ImGui::PushItemWidth(-FLT_MIN);
			return ImGui::SliderInt(std::format("##{}", label).c_str(), &value, min, max);
		}

		bool SliderFloat(const std::string& label, float& value, float min /* = 0.0f */, float max /* = 0.0f */)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(label.c_str());
			ImGui::TableNextColumn();
			ImGui::PushItemWidth(-FLT_MIN);
			return ImGui::SliderFloat(std::format("##{}", label).c_str(), &value, min, max, "%.2f");
		}

		bool Int(const std::string& label, int& value, int step /*= 1*/, int min /*= 0*/, int max /*= 0*/)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(label.c_str());
			ImGui::TableNextColumn();
			ImGui::PushItemWidth(-FLT_MIN);
			return ImGui::DragInt(std::format("##{}", label).c_str(), &value, step, min, max, "%d");
		}

		bool UInt(const std::string& label, uint32_t& value, uint32_t step /*= 1*/, uint32_t min /*= 0*/, uint32_t max /*= 0*/)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(label.c_str());
			ImGui::TableNextColumn();
			ImGui::PushItemWidth(-FLT_MIN);

			int intValue = static_cast<int>(value);
			if (ImGui::DragInt(std::format("##{}", label).c_str(), &intValue, step, min, max, "%d"))
			{
				value = static_cast<uint32_t>(intValue);
				return true;
			}
			return false;
		}


		bool Float(const std::string& label, float& value, float step /*= 0.1f*/, float min /* = 0.0f */, float max /* = 0.0f */, const std::string& format /*= ".2f"*/)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(label.c_str());
			ImGui::TableNextColumn();
			ImGui::PushItemWidth(-FLT_MIN);
			return ImGui::DragFloat(std::format("##{}", label).c_str(), &value, step, min, max, format.c_str());
		}

		bool Float2(const std::string& label, Vector2f& value, float step /*= 0.1f*/, float min /* = 0.0f */, float max /* = 0.0f */)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(label.c_str());
			ImGui::TableNextColumn();
			ImGui::PushItemWidth(-FLT_MIN);
			return ImGui::DragFloat2(std::format("##{}", label).c_str(), &value[0], step, min, max, "%.2f", ImGuiSliderFlags_ColorMarkers);
		}

		bool Float3(const std::string& label, Vector3f& value, float step /*= 0.1f*/, float min /* = 0.0f */, float max /* = 0.0f */)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(label.c_str());
			ImGui::TableNextColumn();
			ImGui::PushItemWidth(-FLT_MIN);
			return ImGui::DragFloat3(std::format("##{}", label).c_str(), &value[0], step, min, max, "%.2f", ImGuiSliderFlags_ColorMarkers);
		}

		bool Float4(const std::string& label, Vector4f& value, float step /* = 0.1f */, float min /* = 0.0f */, float max /* = 0.0f */)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(label.c_str());
			ImGui::TableNextColumn();
			ImGui::PushItemWidth(-FLT_MIN);
			return ImGui::DragFloat4(std::format("##{}", label).c_str(), &value[0], step, min, max, "%.2f", ImGuiSliderFlags_ColorMarkers);
		}

		bool Color3(const std::string& label, Vector3f& color)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(label.c_str());
			ImGui::TableNextColumn();
			ImGui::PushItemWidth(-FLT_MIN);
			return ImGui::ColorEdit3(std::format("##{}", label).c_str(), &color[0]);
		}

		bool Color4(const std::string& label, Vector4f& color)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(label.c_str());
			ImGui::TableNextColumn();
			ImGui::PushItemWidth(-FLT_MIN);
			return ImGui::ColorEdit4(std::format("##{}", label).c_str(), &color[0]);
		}

		bool AssetReference(const std::string& label, const std::string& assetName, const std::string& payloadType, std::string& outDroppedPayload, UICallbackFunc browseFunc /* = nullptr */, UICallbackFunc clearFunc /* = nullptr */)
		{
			bool assetDropped = false;

			ImGui::PushID(label.c_str());

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", label.c_str());

			ImGui::TableNextColumn();

			float buttonSize = ImGui::GetFrameHeight();
			float spacing = ImGui::GetStyle().ItemInnerSpacing.x;

			int numButtons = 0;
			if (browseFunc)
				numButtons++;
			if (clearFunc)
				numButtons++;

			float fieldWidth = ImGui::GetContentRegionAvail().x - ((buttonSize + spacing) * numButtons);

			ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f)); // Left-align the text
			ImGui::Button(assetName.c_str(), ImVec2(fieldWidth, 0));
			ImGui::PopStyleVar();

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType.c_str()))
				{
					outDroppedPayload = (const char*)payload->Data;
					assetDropped = true;
				}
				ImGui::EndDragDropTarget();
			}

			if (browseFunc)
			{
				ImGui::SameLine(0, spacing);
				if (ImGui::Button("...", ImVec2(buttonSize, buttonSize)))
				{
					browseFunc();
				}
			}

			if (clearFunc)
			{
				ImGui::SameLine(0, spacing);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
				if (ImGui::Button("X", ImVec2(buttonSize, buttonSize)))
				{
					clearFunc();
				}
				ImGui::PopStyleColor();
			}

			ImGui::PopID();
			return assetDropped;
		}

		void ActionRow(const std::string& label, const std::string& btn1Label, UICallbackFunc btn1Func, const std::string& btn2Label /* = "" */, UICallbackFunc btn2Func /* = nullptr */)
		{
			ImGui::PushID(label.c_str());

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", label.c_str());

			ImGui::TableNextColumn();

			float actionWidth = ImGui::GetContentRegionAvail().x;
			float spacing = ImGui::GetStyle().ItemInnerSpacing.x;

			if (!btn2Label.empty() && btn2Func != nullptr)
			{
				if (ImGui::Button(btn1Label.c_str(), ImVec2((actionWidth / 2.0f) - (spacing / 2.0f), 0)))
				{
					btn1Func();
				}

				ImGui::SameLine(0, spacing);

				if (ImGui::Button(btn2Label.c_str(), ImVec2((actionWidth / 2.0f) - (spacing / 2.0f), 0)))
				{
					btn2Func();
				}
			}
			else
			{
				if (ImGui::Button(btn1Label.c_str(), ImVec2(actionWidth, 0)))
				{
					btn1Func();
				}
			}

			ImGui::PopID();
		}

		bool DragDropTexture(const std::string& label, UUID textureID, std::string& outDroppedPath, UICallbackFunc clearButtonFunc)
		{
			bool textureChanged = false;

			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			// Prop name
			ImGui::AlignTextToFramePadding();
			ImGui::TextWrapped("%s", label.c_str());

			ImGui::TableNextColumn();

			bool hasValidTexture = textureID != Constants::InvalidUUID;

			auto texture = hasValidTexture
				? Application::Instance().GetAssetManager().GetAsset<Texture2D>(textureID)
				: Application::Instance().GetAssetManager().Load<Texture2D>("Ember-Forge/assets/icons/Empty.png");

			auto id = (void*)(intptr_t)texture->GetID();
			ImGui::Image(id, ImVec2(48, 48), ImVec2(0, 1), ImVec2(1, 0));

			if (ImGui::BeginDragDropTarget())
			{
				std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetTexture);
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType.c_str()))
				{
					outDroppedPath = (const char*)payload->Data;
					textureChanged = true;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();

			//ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
			ImGui::BeginGroup();

			std::string textureName = hasValidTexture
				? std::filesystem::path(texture->GetFilePath()).filename().string()
				: "No Texture Selected";

			ImGui::TextWrapped(textureName.c_str());

			if (hasValidTexture)
			{
				ImGui::PushID(id);

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 4.0f));
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, 2.0f));

				if (ImGui::Button("Clear"))
				{
					clearButtonFunc();
				}

				ImGui::PopStyleVar(2);
				ImGui::PopID();
			}

			ImGui::EndGroup();

			return textureChanged;
		}

		bool BeginComboBox(const std::string& label, const std::string& previewValue)
		{
			// Add a label for the first column and then use UIWidgets.BeginComboBox for next column
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			// Prop name
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", label.c_str());

			ImGui::TableNextColumn();

			return UI::BeginComboBox(std::format("##{}", label), previewValue);
		}

		bool ComboBoxItem(const std::string& itemLabel, bool isSelected)
		{
			// Wrappers for normal UI widget
			return UI::ComboBoxItem(itemLabel, isSelected);
		}

		void EndComboBox()
		{
			UI::EndComboBox();
		}
	}
}