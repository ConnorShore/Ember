#include "efpch.h"
#include "AssetManagerPanel.h"
#include "UI/DragDropTypes.h"
#include "UI/PropertyGrid.h"

#include <Ember/Utils/PlatformUtil.h>
#include <Ember/Event/UIEvent.h>
#include <Ember/Asset/Font.h>
#include <Ember/Core/ProjectManager.h>
#include <Ember/Core/ProjectSerializer.h>
#include <Ember/Scene/Scene.h>
#include <Ember/Scene/SceneManager.h>

#include <Ember-Tools/GLTFImporter.h>

#include <format>
#include <regex>

namespace Ember {

	AssetManagerPanel::AssetManagerPanel(EditorContext* context)
		: Panel("Asset Manager", context), 
		m_RootDirectory(std::filesystem::path("Ember-Forge/assets")),
		m_CurrentDirectory(std::filesystem::path("Ember-Forge/assets"))
	{
	}

	AssetManagerPanel::~AssetManagerPanel()
	{
	}

	void AssetManagerPanel::OnAttach()
	{
		auto& assetManager = Application::Instance().GetAssetManager();

		// Dynamically grab the engine root for the UI icons!
		std::string fileIconPath = (assetManager.GetProjectAssetDirectory() / "icons/File.png").string();
		std::string dirIconPath = (assetManager.GetProjectAssetDirectory() / "icons/Directory.png").string();

		auto fileIcon = assetManager.Load<Texture2D>(fileIconPath);
		auto dirIcon = assetManager.Load<Texture2D>(dirIconPath);
		fileIcon->SetIsEngineAsset(true);
		dirIcon->SetIsEngineAsset(true);

		m_FileTexID = (ImTextureID)(intptr_t)fileIcon->GetID();
		m_DirectoryTexID = (ImTextureID)(intptr_t)dirIcon->GetID();
	}

	void AssetManagerPanel::OnImGuiRender()
	{
		ImGui::Begin(m_Title.c_str());

		RenderPanelControls();
		RenderDirectoryContents();
		RenderAssetPanelContextMenu();

		RenderRenameScenePopup();
		RenderCreateDirectoryPopup();

		ImGui::End();
	}

	void AssetManagerPanel::UpdateRootDirectory(const std::filesystem::path& newDirectory)
	{
		m_RootDirectory = newDirectory;
		m_CurrentDirectory = newDirectory;
	}

	void AssetManagerPanel::RenderPanelControls()
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		if (m_CurrentDirectory != m_RootDirectory)
		{
			if (ImGui::Button("<-"))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}
		}
		else
		{
			ImGui::BeginDisabled();
			ImGui::Button("<-");
			ImGui::EndDisabled();
		}
		ImGui::PopStyleColor();

		ImGui::SameLine();

		// Display the current path
		std::string projPath = "Ember-Forge";
		if (ProjectManager::GetActive() != nullptr)
			projPath = ProjectManager::GetActive()->GetProjectDirectory().string();

		std::string relativePath = std::filesystem::relative(m_CurrentDirectory, projPath).string();
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

				// Filter hidden files
				if (std::find(m_HiddenFiles.begin(), m_HiddenFiles.end(), fileNameStr) != m_HiddenFiles.end())
					continue;

				// Filter hidden files that are a wildcard match (e.g. *.ebproj)
				if (std::any_of(m_HiddenFiles.begin(), m_HiddenFiles.end(), [&](const std::string& hiddenPattern) {
					if (hiddenPattern.find('*') != std::string::npos)
					{
						std::string pattern = std::regex_replace(hiddenPattern, std::regex("\\*"), ".*");
						return std::regex_match(fileNameStr, std::regex(pattern));
					}
					return false;
				}))
				{
					continue;
				}

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

			// This completely separates the files from the drop target.
			ImGui::EndTable();
		}

		// Create the dropzone
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::Dummy(ImGui::GetContentRegionAvail());
		ImGui::PopStyleVar();

		// 3. HANDLE THE DROP TARGETS
		if (ImGui::BeginDragDropTarget())
		{
			// Target A: Moving an existing Asset Prefab around the filesystem
			std::string prefabPayload = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetPrefab);
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(prefabPayload.c_str()))
			{
				std::string filePath = std::string((char*)payload->Data, payload->DataSize);
				std::filesystem::path destPath = m_CurrentDirectory / std::filesystem::path(filePath).filename();
				std::error_code ec;
				std::filesystem::copy(filePath, destPath, std::filesystem::copy_options::overwrite_existing, ec);
				if (ec)
				{
					EB_CORE_ERROR("Failed to copy asset to '{0}': {1}", destPath.string(), ec.message());
				}
				else
				{
					EB_CORE_INFO("Successfully copied asset to: {0}", destPath.string());
					Application::Instance().GetAssetManager().Load<Prefab>(destPath.string());
				}
			}

			// Target B: Dropping a Scene Entity to create a BRAND NEW Prefab!
			std::string entityPayload = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::SceneEntity);
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(entityPayload.c_str()))
			{
				UUID entityUUID = *(const UUID*)payload->Data;

				if (m_Context && m_Context->ActiveScene())
				{
					Entity entity = m_Context->ActiveScene()->GetEntity(entityUUID);
					if (entity != Constants::Entities::InvalidEntityID)
					{
						// Construct the save path using the active directory
						std::string filePath = (m_CurrentDirectory / (entity.GetName() + ".ebprefab")).string();

						// Create the prefab asset and save it to disk
						SharedPtr<Prefab> prefab = m_Context->ActiveScene()->CreatePrefab(entity, filePath);
						if (prefab == nullptr)
						{
							auto evt = UINotificationEvent(std::format("Failed to create prefab from entity {}!", entity.GetName()), UINotificationEvent::Error);
							m_Context->EventCallback(evt);
							return;
						}

						// Success notification
						auto evt = UINotificationEvent(std::format("Prefab {} created!", prefab->GetName()));
						m_Context->EventCallback(evt);
					}
				}
			}

			ImGui::EndDragDropTarget();
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

		// TODO: Make this a drop target for moving files into the directory (and directories into directories)
	}

	void AssetManagerPanel::RenderFileEntryContextMenu(const std::filesystem::directory_entry& entry)
	{
		const std::filesystem::path filePath = entry.path();
		const std::filesystem::path fileName = entry.path().filename();

		if (ImGui::BeginPopupContextItem())
		{
			// Render asset-specific options based on the file extension
			auto assetType = DragDropUtils::ExtensionToDragDropPayloadType(filePath.extension().string());
			switch (assetType)
			{
			case DragDropPayloadType::AssetModel:
				RenderModelOptions(filePath.string());
				break;
			case DragDropPayloadType::AssetTexture:
				RenderTextureOptions(filePath.string());
				break;
			case DragDropPayloadType::AssetPrefab:
				RenderPrefabOptions(filePath.string());
				break;
			case DragDropPayloadType::Scene:
				RenderSceneOptions(filePath.string());
				break;
			case DragDropPayloadType::AssetAudioClip:
				RenderAudioClipOptions(filePath.string());
				break;
			case DragDropPayloadType::AssetFont:
				RenderFontOptions(filePath.string());
				break;
			case DragDropPayloadType::AssetShader:
				RenderShaderOptions(filePath.string());
				break;
			case DragDropPayloadType::AssetScript:
				RenderScriptOptions(filePath.string());
				break;
			}

			// Common options for all asset types
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
			if (ImGui::MenuItem("New Directory"))
			{
				m_ShowCreateDirectoryPopup = true;
			}

			if (ImGui::BeginMenu("Import Asset"))
			{
				SharedPtr<Asset> asset = nullptr;
				if (ImGui::MenuItem("Model"))
				{
					std::string modelFileTypes = DragDropUtils::DragDropPayloadTypeToExtension(DragDropPayloadType::AssetModel);
					std::string sourceFile = FileDialog::OpenFile("", std::format("Model Files ({})", modelFileTypes).c_str(), modelFileTypes.c_str());

					// Need to convert to GLTFImporter and model cooking for static vs skeletal meshes
					if (!sourceFile.empty())
					{
						//auto reportOpt = ModelImporter::CookModel(sourceFile, m_CurrentDirectory.string());
						auto reportOpt = GLTFImporter::CookModel(sourceFile, m_CurrentDirectory.string());
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

							for (auto& anim : reportOpt->Animations)
								am.Load<Animation>(anim.id, anim.name, anim.path, false);

							if (reportOpt->Skeleton.id != Constants::InvalidUUID)
								am.Load<Skeleton>(reportOpt->Skeleton.id, reportOpt->Skeleton.name, reportOpt->Skeleton.path, false);

							// Load the final model asset
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
					{
						asset = Application::Instance().GetAssetManager().Load<Texture2D>(file);
						asset->SetIsEngineAsset(false);
					}
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::MenuItem("Shader"))
				{
					std::string modelFileTypes = DragDropUtils::DragDropPayloadTypeToExtension(DragDropPayloadType::AssetShader);
					std::string file = SelectAndLoadFile(std::format("Shader Files ({})", modelFileTypes).c_str(), modelFileTypes.c_str());
					if (!file.empty())
					{
						asset = Application::Instance().GetAssetManager().Load<Shader>(file);
						asset->SetIsEngineAsset(false);
					}
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::MenuItem("Script"))
				{
					std::string modelFileTypes = DragDropUtils::DragDropPayloadTypeToExtension(DragDropPayloadType::AssetScript);
					std::string file = SelectAndLoadFile(std::format("Script Files ({})", modelFileTypes).c_str(), modelFileTypes.c_str());
					if (!file.empty())
					{
						asset = Application::Instance().GetAssetManager().Load<Script>(file);
						asset->SetIsEngineAsset(false);
					}
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::MenuItem("Prefab"))
				{
					std::string modelFileTypes = DragDropUtils::DragDropPayloadTypeToExtension(DragDropPayloadType::AssetPrefab);
					std::string file = SelectAndLoadFile(std::format("Prefab Files ({})", modelFileTypes).c_str(), modelFileTypes.c_str());
					if (!file.empty())
					{
						asset = Application::Instance().GetAssetManager().Load<Script>(file);
						asset->SetIsEngineAsset(false);
					}
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::MenuItem("Font"))
				{
					std::string modelFileTypes = DragDropUtils::DragDropPayloadTypeToExtension(DragDropPayloadType::AssetFont);
					std::string file = SelectAndLoadFile(std::format("Font Files ({})", modelFileTypes).c_str(), modelFileTypes.c_str());
					if (!file.empty())
					{
						asset = Application::Instance().GetAssetManager().Load<Font>(file);
						asset->SetIsEngineAsset(false);
					}
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Audio"))
				{
					std::string modelFileTypes = DragDropUtils::DragDropPayloadTypeToExtension(DragDropPayloadType::AssetAudioClip);
					std::string file = SelectAndLoadFile(std::format("Audio Files ({})", modelFileTypes).c_str(), modelFileTypes.c_str());
					if (!file.empty())
					{
						asset = Application::Instance().GetAssetManager().Load<AudioClip>(file);
						asset->SetIsEngineAsset(false);
					}
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

	void AssetManagerPanel::RenderTextureOptions(const std::string& filePath)
	{

	}

	void AssetManagerPanel::RenderModelOptions(const std::string& filePath)
	{

	}

	void AssetManagerPanel::RenderPrefabOptions(const std::string& filePath)
	{

	}

	void AssetManagerPanel::RenderSceneOptions(const std::string& filePath)
	{
		if (ImGui::MenuItem("Rename"))
		{
			m_RenameSceneOldFilePath = filePath;
			m_RenameSceneNewName = std::filesystem::path(filePath).stem().string();
			m_ShowRenameScenePopup = true;
		}
	}

	void AssetManagerPanel::RenderRenameScenePopup()
	{
		if (m_ShowRenameScenePopup)
		{
			ImGui::OpenPopup("Rename Scene");
			m_ShowRenameScenePopup = false;
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(325, 150), ImGuiCond_Appearing);

		if (ImGui::BeginPopupModal("Rename Scene", NULL, ImGuiWindowFlags_NoSavedSettings))
		{
			if (UI::PropertyGrid::Begin("RenameSceneTable"))
			{
				UI::PropertyGrid::InputText("Scene Name", m_RenameSceneNewName);

				UI::PropertyGrid::End();
			}

			ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()));

			std::string currentStem = std::filesystem::path(m_RenameSceneOldFilePath).stem().string();
			bool isValid = !m_RenameSceneNewName.empty() && m_RenameSceneNewName != currentStem;
			if (!isValid)
				ImGui::BeginDisabled();

			if (ImGui::Button("Rename", ImVec2(120, 0)))
			{
				RenameScene();
				ImGui::CloseCurrentPopup();
			}

			if (!isValid)
				ImGui::EndDisabled();

			ImGui::SameLine();

			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				m_RenameSceneNewName.clear();
				m_RenameSceneOldFilePath.clear();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void AssetManagerPanel::RenameScene()
	{
		std::filesystem::path oldPath = m_RenameSceneOldFilePath;
		std::filesystem::path newPath = oldPath.parent_path() / (m_RenameSceneNewName + ".ebs");

		auto sendNotification = [&](const std::string& msg, UINotificationEvent::Severity sev)
		{
			UINotificationEvent evt(msg, sev);
			m_Context->EventCallback(evt);
		};

		if (!std::filesystem::exists(oldPath))
		{
			sendNotification("Scene file does not exist!", UINotificationEvent::Severity::Error);
			return;
		}

		if (std::filesystem::exists(newPath))
		{
			sendNotification("A scene with that name already exists!", UINotificationEvent::Severity::Error);
			return;
		}

		auto& assetManager = Application::Instance().GetAssetManager();

		// Look up the asset (if registered) before we touch the disk so we still have the old path key.
		SharedPtr<Scene> sceneAsset;
		if (assetManager.ContainsAssetWithPath(oldPath.string()))
			sceneAsset = assetManager.GetAssetByPath<Scene>(oldPath.string());

		std::error_code ec;
		std::filesystem::rename(oldPath, newPath, ec);
		if (ec)
		{
			sendNotification(std::format("Failed to rename scene file: {}", ec.message()), UINotificationEvent::Severity::Error);
			return;
		}

		// Update the asset registry / asset state if the scene was tracked.
		if (sceneAsset)
		{
			if (!assetManager.RenameAsset(sceneAsset->GetUUID(), m_RenameSceneNewName, newPath.string()))
			{
				sendNotification("Failed to update scene asset entry!", UINotificationEvent::Severity::Error);
				// Best-effort: revert the file move to keep state consistent.
				std::error_code revertEc;
				std::filesystem::rename(newPath, oldPath, revertEc);
				return;
			}

			// If this scene is the project's start scene, refresh the project config and persist it.
			auto project = ProjectManager::GetActive();
			if (project)
			{
				auto startPath = std::filesystem::absolute(project->GetStartScenePath()).string();
				auto oldAbs = std::filesystem::absolute(oldPath).string();
				auto newAbs = std::filesystem::absolute(newPath).string();
				if (startPath == newAbs || startPath == oldAbs)
				{
					project->SetStartScene(sceneAsset->GetUUID());
					ProjectSerializer serializer(project);
					serializer.Serialize(project->GetProjectFilePath().string());
				}
			}
		}

		sendNotification(std::format("Scene renamed to '{}'", m_RenameSceneNewName), UINotificationEvent::Severity::Info);

		m_RenameSceneNewName.clear();
		m_RenameSceneOldFilePath.clear();
	}

	void AssetManagerPanel::RenderCreateDirectoryPopup()
	{
		if (m_ShowCreateDirectoryPopup)
		{
			ImGui::OpenPopup("Create Directory");
			m_ShowCreateDirectoryPopup = false;
		}

		if (ImGui::BeginPopupModal("Create Directory", NULL, ImGuiWindowFlags_NoSavedSettings))
		{
			char directoryNameBuffer[128] = "NewFolder";
			if (ImGui::InputText("Name", directoryNameBuffer, sizeof(directoryNameBuffer)))
			{
				m_NewDirectoryName = std::string(directoryNameBuffer);
				// Remove any invalid characters from the directory name
				std::string invalidChars = "\\/:?\"<>|";
				for (char c : invalidChars)
					m_NewDirectoryName.erase(std::remove(m_NewDirectoryName.begin(), m_NewDirectoryName.end(), c), m_NewDirectoryName.end());
			}

			ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()));
			bool isValid = !m_NewDirectoryName.empty();
			if (!isValid)
				ImGui::BeginDisabled();
			if (ImGui::Button("Create", ImVec2(120, 0)))
			{
				std::filesystem::path newDirPath = m_CurrentDirectory / m_NewDirectoryName;
				std::error_code ec;
				std::filesystem::create_directory(newDirPath, ec);
				if (ec)
				{
					auto evt = UINotificationEvent(std::format("Failed to create directory '{}': {}", m_NewDirectoryName, ec.message()), UINotificationEvent::Error);
					m_Context->EventCallback(evt);
				}
				else
				{
					auto evt = UINotificationEvent(std::format("Directory '{}' created!", m_NewDirectoryName));
					m_Context->EventCallback(evt);
				}
				m_NewDirectoryName.clear();
				ImGui::CloseCurrentPopup();
			}
			if (!isValid)
				ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				m_NewDirectoryName.clear();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	void AssetManagerPanel::RenderAudioClipOptions(const std::string& filePath)
	{

	}

	void AssetManagerPanel::RenderFontOptions(const std::string& filePath)
	{

	}

	void AssetManagerPanel::RenderShaderOptions(const std::string& filePath)
	{

	}

	void AssetManagerPanel::RenderScriptOptions(const std::string& filePath)
	{
		if (ImGui::MenuItem("Edit Script"))
		{
			ScriptEditor::OpenScript(filePath);
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