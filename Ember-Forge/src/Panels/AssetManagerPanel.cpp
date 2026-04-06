#include "efpch.h"
#include "AssetManagerPanel.h"
#include "UI/DragDropTypes.h"
#include "UI/PropertyGrid.h"

#include <Ember-Tools/ModelImporter.h>

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
		auto fileIcon = assetManager.Load<Texture2D>("Ember-Forge/assets/icons/File.png");
		auto dirIcon = assetManager.Load<Texture2D>("Ember-Forge/assets/icons/Directory.png");

		m_FileTexID = (ImTextureID)(intptr_t)fileIcon->GetID();
		m_DirectoryTexID = (ImTextureID)(intptr_t)dirIcon->GetID();
	}

	void AssetManagerPanel::OnImGuiRender()
	{
		ImGui::Begin(m_Title.c_str());

		RenderPanelControls();
		RenderDirectoryContents();
		RenderAssetPanelContextMenu();

		ImGui::End();
	}

	void AssetManagerPanel::UpdateAssetDirectory(const std::filesystem::path& newDirectory)
	{
		m_AssetDirectory = newDirectory;
		m_CurrentDirectory = newDirectory;
	}

	void AssetManagerPanel::RenderPanelControls()
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		if (m_CurrentDirectory != m_AssetDirectory)
		{
			if (ImGui::Button("<- Back"))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}
		}
		else
		{
			ImGui::BeginDisabled();
			ImGui::Button("<- Back");
			ImGui::EndDisabled();
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();

		// Display the current path
		std::string relativePath = std::filesystem::relative(m_CurrentDirectory, "Ember-Forge").string();
		ImGui::TextDisabled("%s", relativePath.c_str());

		// Size slider
		float sliderWidth = 150.0f;
		float labelWidth = ImGui::CalcTextSize("Icon Size").x;
		float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
		float totalRightWidth = sliderWidth + labelWidth + spacing;

		// Calculate where the right-aligned item should start
		float currentCursorX = ImGui::GetCursorPosX();
		float rightAlignedX = ImGui::GetWindowContentRegionMax().x - totalRightWidth;

		if (rightAlignedX > currentCursorX)
		{
			ImGui::SameLine(rightAlignedX);
		}
		else
		{
			ImGui::SameLine();
		}

		ImGui::Text("Icon Size");
		ImGui::SameLine(0, spacing);
		ImGui::SetNextItemWidth(sliderWidth);

		ImGui::SliderInt("##IconSize", &m_IconSize, 20, 400);

		ImGui::Separator();
	}

	void AssetManagerPanel::RenderDirectoryContents()
	{
		// Calculate how many columns we can fit
		float padding = 16.0f;
		float cellSize = m_IconSize + padding;
		float panelWidth = ImGui::GetContentRegionAvail().x;
		int numColumns = Math::Max((int)(panelWidth / cellSize), 1);

		if (ImGui::BeginTable("AssetBrowserTable", numColumns, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingFixedFit))
		{
			std::filesystem::directory_iterator it(m_CurrentDirectory);

			for (const auto& entry : it)
			{
				std::string filePath = entry.path().string();
				std::filesystem::path fileName = entry.path().filename();
				std::string fileNameStr = fileName.string();

				if (std::find(m_HiddenFiles.begin(), m_HiddenFiles.end(), fileNameStr) != m_HiddenFiles.end())
					continue;

				ImGui::TableNextColumn();

				// Push the unique file path as an ID to prevent ImGui button conflicts
				ImGui::PushID(filePath.c_str());

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 0.5f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));

				if (entry.is_directory())
				{
					RenderDirectoryEntry(entry);
				}
				else
				{
					RenderFileEntry(entry);
				}

				ImGui::PopStyleColor(3);

				float textWidth = ImGui::CalcTextSize(fileNameStr.c_str()).x;

				// Only shift right if the text is smaller than the icon
				if (textWidth < m_IconSize)
				{
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (m_IconSize - textWidth) * 0.5f);
				}

				ImGui::TextWrapped("%s", fileNameStr.c_str());

				ImGui::PopID();
			}

			ImGui::EndTable();
		}
	}

	void AssetManagerPanel::RenderFileEntry(const std::filesystem::directory_entry& entry)
	{
		const std::filesystem::path filePath = entry.path();
		const std::filesystem::path fileName = entry.path().filename();
		const std::string fileNameStr = fileName.string();

		if (ImGui::ImageButton(fileNameStr.c_str(), m_FileTexID, ImVec2(static_cast<float>(m_IconSize), static_cast<float>(m_IconSize)), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f)))
		{
			// TODO: Select the item
		}
		
		RenderFileEntryContextMenu(entry);

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			auto filePathAbs = std::filesystem::absolute(filePath);
			auto payloadType = DragDropUtils::ExtensionToDragDropPayloadType(filePathAbs.extension().string());
			auto payloadStr = DragDropUtils::DragDropPayloadTypeToString(payloadType);

			ImGui::SetDragDropPayload(payloadStr.c_str(), filePathAbs.string().c_str(), filePathAbs.string().size() + 1);
			ImGui::Image(m_FileTexID, ImVec2(64, 64), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
			ImGui::EndDragDropSource();
		}
	}

	void AssetManagerPanel::RenderDirectoryEntry(const std::filesystem::directory_entry& entry)
	{
		const std::filesystem::path fileName = entry.path().filename();
		const std::string fileNameStr = fileName.string();

		ImGui::ImageButton(fileNameStr.c_str(), m_DirectoryTexID, ImVec2(static_cast<float>(m_IconSize), static_cast<float>(m_IconSize)), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			m_CurrentDirectory /= fileName;
		}
	}

	void AssetManagerPanel::RenderFileEntryContextMenu(const std::filesystem::directory_entry& entry)
	{
		const std::filesystem::path filePath = entry.path();
		const std::filesystem::path fileName = entry.path().filename();

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete"))
			{
				Application::Instance().GetAssetManager().RemoveAsset(filePath.string());
				std::error_code ec;
				std::filesystem::remove(filePath, ec);

				if (ec)
				{
					EB_CORE_ERROR("Failed to delete asset '{0}': {1}", fileName.string(), ec.message());
				}
				else
				{
					EB_CORE_INFO("Successfully deleted asset: {0}", fileName.string());
				}
			}

			ImGui::EndPopup();
		}
	}

	void AssetManagerPanel::RenderAssetPanelContextMenu()
	{
		if (ImGui::BeginPopupContextWindow("AssetManagerPanelContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::BeginMenu("Import Asset"))
			{
				SharedPtr<Asset> asset = nullptr;
				if (ImGui::MenuItem("Model"))
				{
					std::string modelFileTypes = DragDropUtils::DragDropPayloadTypeToExtension(DragDropPayloadType::AssetModel);
					std::string sourceFile = FileDialog::OpenFile("", modelFileTypes.c_str());

					if (!sourceFile.empty())
					{
						auto reportOpt = ModelImporter::CookModel(sourceFile, m_CurrentDirectory.string());

						if (reportOpt.has_value())
						{
							auto& am = Application::Instance().GetAssetManager();

							// Load the newly cooked assets from the project directory
							for (auto& tex : reportOpt->Textures)
								am.Load<Texture2D>(tex.id, tex.name, tex.path, false);

							for (auto& mat : reportOpt->Materials)
								am.Load<MaterialBase>(mat.id, mat.name, mat.path, false);

							for (auto& mesh : reportOpt->Meshes)
								am.Load<Mesh>(mesh.id, mesh.name, mesh.path, false);

							asset = am.Load<Model>(reportOpt->Model.id, reportOpt->Model.name, reportOpt->Model.path, false);
						}
					}
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::MenuItem("Texture"))
				{
					std::string modelFileTypes = DragDropUtils::DragDropPayloadTypeToExtension(DragDropPayloadType::AssetTexture);
					std::string file = SelectAndLoadFile(std::format("Texture Files ({})", modelFileTypes).c_str(), modelFileTypes.c_str());
					if (!file.empty())
						asset = Application::Instance().GetAssetManager().Load<Texture2D>(file);
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
					EB_CORE_INFO("Successfully imported asset: {0}", asset->GetName());
				}

				ImGui::EndMenu();
			}

			ImGui::EndPopup();
		}
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