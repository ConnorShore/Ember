#include "AssetManagerPanel.h"
#include "Utils/DragDropTypes.h"

#include <format>

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
			std::string filePath = entry.path().string();
			std::filesystem::path fileName = entry.path().filename();
			std::string fileNameStr = entry.path().filename().string();

			if (std::find(m_HiddenFiles.begin(), m_HiddenFiles.end(), fileNameStr) != m_HiddenFiles.end())
				continue;

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.f));

			EB_CORE_WARN("Comparing exact bytes: [{0}] vs [{1}]", m_HiddenFiles[0], fileNameStr);
			EB_CORE_WARN("Lengths - Array: {0}, OS: {1}", m_HiddenFiles[0].length(), fileNameStr.length());
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
				if (ImGui::ImageButton(fileNameStr.c_str(), m_FileTexID, ImVec2(m_IconSize, m_IconSize), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f)))
				{
					// TODO: Select the item
				}
				if (ImGui::BeginPopupContextItem())
				{
					if (ImGui::MenuItem("Delete"))
					{
						Application::Instance().GetAssetManager().RemoveAsset(filePath);
						std::error_code ec;
						std::filesystem::remove(filePath, ec);

						if (ec)
						{
							EB_CORE_ERROR("Failed to delete asset '{0}': {1}", fileNameStr, ec.message());
						}
						else
						{
							EB_CORE_INFO("Successfully deleted asset: {0}", fileNameStr);
						}
					}

					ImGui::EndPopup();
				}

				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
				{
					auto filePathAbs = std::filesystem::absolute(filePath);
					auto payloadType = DragDropUtils::ExtensionToDragDropPayloadType(filePathAbs.extension().string());
					auto payloadStr = DragDropUtils::DragDropPayloadTypeToString(payloadType);
					EB_CORE_INFO("Dragging asset '{}' with payload type '{}'", fileNameStr, payloadStr);
					ImGui::SetDragDropPayload(payloadStr.c_str(), filePathAbs.string().c_str(), filePathAbs.string().size() + 1);
					ImGui::Image(m_FileTexID, ImVec2(64, 64), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
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

		if (ImGui::BeginPopupContextWindow("AssetManagerPanelContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::BeginMenu("Import Asset"))
			{
				SharedPtr<Asset> asset = nullptr;
				if (ImGui::MenuItem("Model"))
				{
					std::string modelFileTypes = DragDropUtils::DragDropPayloadTypeToExtension(DragDropPayloadType::AssetModel);
					std::string file = SelectAndLoadFile(std::format("Model Files ({})", modelFileTypes).c_str(), modelFileTypes.c_str());
					if (!file.empty())
						asset = Application::Instance().GetAssetManager().Load<Model>(file);
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::MenuItem("Texture"))
				{
					std::string modelFileTypes = DragDropUtils::DragDropPayloadTypeToExtension(DragDropPayloadType::AssetTexture);
					std::string file = SelectAndLoadFile(std::format("Texture Files ({})", modelFileTypes).c_str(), modelFileTypes.c_str());
					if (!file.empty())
						asset = Application::Instance().GetAssetManager().Load<Texture>(file);
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::MenuItem("Shader"))
				{
					std::string modelFileTypes = DragDropUtils::DragDropPayloadTypeToExtension(DragDropPayloadType::AssetShader);
					std::string file = SelectAndLoadFile(std::format("Shader Files ({})", modelFileTypes).c_str(), modelFileTypes.c_str());
					if (!file.empty())
						asset = Application::Instance().GetAssetManager().Load<Shader>(file);
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::MenuItem("Script"))
				{
					std::string modelFileTypes = DragDropUtils::DragDropPayloadTypeToExtension(DragDropPayloadType::AssetScript);
					std::string file = SelectAndLoadFile(std::format("Script Files ({})", modelFileTypes).c_str(), modelFileTypes.c_str());
					if (!file.empty())
						asset = Application::Instance().GetAssetManager().Load<Script>(file);
					ImGui::CloseCurrentPopup();
				}

				if (asset != nullptr)
				{
					asset->SetIsEngineAsset(false);
					EB_CORE_INFO("Successfully imported asset: {0}", asset->GetName());
				}

				ImGui::EndMenu();
			}

			ImGui::EndPopup();
		}

		ImGui::End();
	}

	void AssetManagerPanel::UpdateAssetDirectory(const std::filesystem::path& newDirectory)
	{
		m_AssetDirectory = newDirectory;
		m_CurrentDirectory = newDirectory;
	}

	std::string AssetManagerPanel::SelectAndLoadFile(const std::string& name, const std::string& type)
	{
		std::string file = FileDialog::OpenFile(m_CurrentDirectory.string().c_str(), name.c_str(), type.c_str());
		if (file.empty())
			return "";

		// If file already exists in this directory, just return the path
		if (std::filesystem::exists(m_CurrentDirectory / std::filesystem::path(file).filename()))
			return file;

		// Copy the file to the current directory
		std::filesystem::path destPath = m_CurrentDirectory / std::filesystem::path(file).filename();
		std::filesystem::copy_file(file, destPath, std::filesystem::copy_options::overwrite_existing);
		return destPath.string();
	}

}