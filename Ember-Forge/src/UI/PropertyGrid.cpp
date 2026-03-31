#include "PropertyGrid.h"
#include "DragDropTypes.h"

namespace Ember {
	namespace UI::PropertyGrid {

		// Property Grid Layout
		bool Begin(const std::string& id)
		{
			ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable;
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
		bool HeaderWithActionButton(const std::string& headerLabel, const std::string& buttonLabel)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(headerLabel.c_str());

			// Actions
			ImGui::TableNextColumn();
			float buttonWidth = ImGui::CalcTextSize(buttonLabel.c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f;
			float posX = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - buttonWidth;
			ImGui::SetCursorPosX(posX);

			return ImGui::Button(buttonLabel.c_str());
		}

		// Property Grid Widgets

		bool Slider(const std::string& label, float& value, float min /* = 0.0f */, float max /* = 0.0f */)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(label.c_str());
			ImGui::TableNextColumn();
			ImGui::PushItemWidth(-FLT_MIN);
			return ImGui::SliderFloat(std::format("##{}", label).c_str(), &value, min, max, "%.2f");
		}

		bool Float(const std::string& label, float& value, float step /*= 0.1f*/, float min /* = 0.0f */, float max /* = 0.0f */)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text(label.c_str());
			ImGui::TableNextColumn();
			ImGui::PushItemWidth(-FLT_MIN);
			return ImGui::DragFloat(std::format("##{}", label).c_str(), &value, step, min, max, "%.2f");
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
				? Application::Instance().GetAssetManager().GetAsset<Texture>(textureID)
				: Application::Instance().GetAssetManager().Load<Texture>("Ember-Forge/assets/icons/Empty.png");

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
	}
}