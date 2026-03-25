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


		if (m_CurrentDirectory != m_AssetDirectory)
		{
			if (ImGui::Button("<-"))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}
		}

		std::filesystem::directory_iterator it(m_CurrentDirectory);
		int size = 50;
		float padding = 10.0f;
		float currentPos = ImGui::GetCursorPosX();
		for (const auto& entry : it)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.f));

			if (entry.is_directory())
			{
				if (ImGui::ImageButton(entry.path().filename().string().c_str(), m_DirectoryTexID, ImVec2(size, size), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f)))
				{
					m_CurrentDirectory = entry.path();
				}
			}
			else
			{
				if (ImGui::ImageButton(entry.path().filename().string().c_str(), m_FileTexID, ImVec2(size, size), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f)))
				{
				}
			}

			ImGui::PopStyleColor(3);
			ImGui::Text(entry.path().stem().string().c_str());
		}


		ImGui::End();
	}

}