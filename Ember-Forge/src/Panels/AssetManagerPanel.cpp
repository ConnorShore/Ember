#include "AssetManagerPanel.h"

namespace Ember {


	AssetManagerPanel::AssetManagerPanel(EditorContext* context)
		: Panel("Asset Manager", context), 
		m_AssetDirectory(std::filesystem::path("Ember-Forge/assets")), 
		m_CurrentDirectory(std::filesystem::path("Ember-Forge/assets"))
	{
	}

	AssetManagerPanel::~AssetManagerPanel()
	{
	}

	void AssetManagerPanel::OnAttach()
	{
		auto& assetManager = Application::Instance().GetAssetManager();
		auto fileIcon = assetManager.Load<Texture>("Ember-Forge/assets/icons/File.png");
		auto dirIcon = assetManager.Load<Texture>("Ember-Forge/assets/icons/Directory.png");
		m_FileTexID = (ImTextureID)(intptr_t)fileIcon->GetID();
		m_DirectoryTexID = (ImTextureID)(intptr_t)dirIcon->GetID();
	}

	void AssetManagerPanel::OnImGuiRender()
	{
		ImGui::Begin(m_Title.c_str());

		ImGui::BeginTable("##SliderTable", 2, ImGuiTableFlags_SizingFixedFit);
		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Icon Size");
		ImGui::TableNextColumn();
		ImGui::SliderInt("##IconSize", &m_IconSize, 20, 400);
		ImGui::EndTable();

		if (m_CurrentDirectory != m_AssetDirectory)
		{
			if (ImGui::Button("<-"))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}
		}

		std::filesystem::directory_iterator it(m_CurrentDirectory);
		int numColumns = Math::Max((int)(ImGui::GetWindowContentRegionMax().x / (m_IconSize)), 1);

		ImGui::Columns(numColumns, nullptr, false);
		for (const auto& entry : it)
		{
			//float currentPos = ImGui::GetCursorPosX();
			std::string filePath = entry.path().string();
			std::filesystem::path fileName = entry.path().filename();
			std::string fileNameStr = entry.path().filename().string();

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.f));

			if (entry.is_directory())
			{
				ImGui::ImageButton(fileNameStr.c_str(), m_DirectoryTexID, ImVec2(m_IconSize, m_IconSize), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					m_CurrentDirectory /= fileName;
				}
			}
			else
			{
				ImGui::ImageButton(fileNameStr.c_str(), m_FileTexID, ImVec2(m_IconSize, m_IconSize), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
				{
					if (ImGui::GetDragDropPayload() == NULL)
					{
						auto filePathAbs = std::filesystem::absolute(filePath).string();
						ImGui::SetDragDropPayload("ASSET_FILE", filePathAbs.c_str(), filePathAbs.size() + 1);
					}
					ImGui::EndDragDropSource();
				}
			}

			ImGui::PopStyleColor(3);

			// Center text in column
			ImVec2 textSize = ImGui::CalcTextSize(fileNameStr.c_str());
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (m_IconSize - textSize.x) / 2);
			ImGui::TextWrapped(fileNameStr.c_str());

			// Decide if we should wrap to next line or not
			if (ImGui::GetColumnIndex() == numColumns - 1)
			{
				ImGui::NewLine();
				ImGui::NextColumn();
			}
			else
			{
				ImGui::NextColumn();
			}
		}

		ImGui::Columns(1);

		ImGui::End();
	}

}