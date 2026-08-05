#include "efpch.h"
#include "AssetManagerPanel.h"
#include "UI/DragDropTypes.h"
#include "UI/PropertyGrid.h"

#include <Ember/Utils/PlatformUtil.h>
#include <Ember/Event/UIEvent.h>
#include <Ember/Asset/Serializers/AssetRegistrySerializer.h>
#include <Ember/Asset/Font.h>
#include <Ember/Asset/NavigationMeshData.h>
#include <Ember/Core/ProjectManager.h>
#include <Ember/Core/ProjectSerializer.h>
#include <Ember/Scene/Scene.h>
#include <Ember/Scene/SceneManager.h>

#include <Ember-Tools/GLTFImporter.h>

#include <cctype>
#include <format>
#include <regex>

namespace {

	std::string ToLowerCopy(std::string value)
	{
		std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return value;
	}

	bool PathsEqual(const std::filesystem::path& lhs, const std::filesystem::path& rhs)
	{
		std::error_code ec;
		if (std::filesystem::equivalent(lhs, rhs, ec))
			return true;

		return std::filesystem::absolute(lhs).lexically_normal() == std::filesystem::absolute(rhs).lexically_normal();
	}

	Ember::DragDropPayloadType AssetPayloadTypeForPath(const std::filesystem::path& filePath)
	{
		return Ember::DragDropUtils::ExtensionToDragDropPayloadType(ToLowerCopy(filePath.extension().string()));
	}

}

namespace Ember {

	// Identifiers for drag and drop payloads
	constexpr const char* DragDropFile = "File";
	constexpr const char* DragDropDirectory = "Directory";

	// Left empty until a project opens: EditorLayer calls UpdateRootDirectory then, and the panel is not
	// rendered before that. It used to default to the editor's own asset folder, which an installed
	// build has no business browsing.
	AssetManagerPanel::AssetManagerPanel(EditorContext* context)
		: Panel("Asset Manager", context)
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

		float availableWidth = ImGui::GetContentRegionAvail().x;
		float maxFolderPaneWidth = std::max(160.0f, availableWidth - 180.0f);
		m_FolderPaneWidth = std::clamp(m_FolderPaneWidth, 160.0f, maxFolderPaneWidth);

		ImGui::BeginChild("AssetManagerFolderPane", ImVec2(m_FolderPaneWidth, 0.0f), true);
		RenderFolderHierarchy();
		ImGui::EndChild();

		ImGui::SameLine(0.0f, 0.0f);
		ImGui::InvisibleButton("##AssetManagerSplitter", ImVec2(4.0f, ImGui::GetContentRegionAvail().y));
		if (ImGui::IsItemActive())
			m_FolderPaneWidth += ImGui::GetIO().MouseDelta.x;
		if (ImGui::IsItemHovered() || ImGui::IsItemActive())
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

		ImGui::SameLine(0.0f, 0.0f);
		ImGui::BeginChild("AssetManagerContentPane", ImVec2(0.0f, 0.0f), true);
		RenderPanelControls();

		ImGui::BeginChild("AssetManagerTilePane", ImVec2(0.0f, 0.0f), false);
		RenderDirectoryContents();
		RenderAssetPanelContextMenu();
		ImGui::EndChild();
		ImGui::EndChild();

		RenderRenameScenePopup();
		RenderCreateDirectoryPopup();
		RenderDeleteConfirmPopup();
		RenderSkeletonMaskOptionsPopup();

		ImGui::End();
	}

	void AssetManagerPanel::UpdateRootDirectory(const std::filesystem::path& newDirectory)
	{
		m_RootDirectory = newDirectory;
		m_CurrentDirectory = newDirectory;
	}

	void AssetManagerPanel::RenderPanelControls()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGuiStyle& style = ImGui::GetStyle();

		float searchWidth = 220.0f;
		float sliderWidth = 140.0f;
		float tileLabelWidth = ImGui::CalcTextSize("Tile Size").x;
		float newFolderWidth = ImGui::CalcTextSize("+ Folder").x + style.FramePadding.x * 2.0f;
		float rightColumnWidth = newFolderWidth + style.ItemSpacing.x + searchWidth + style.ItemSpacing.x + tileLabelWidth + style.ItemInnerSpacing.x + sliderWidth;

		if (ImGui::BeginTable("AssetManagerHeaderRow", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
		{
			ImGui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthFixed, rightColumnWidth);
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			if (m_CurrentDirectory != m_RootDirectory)
			{
				if (ImGui::Button("<", ImVec2(28.0f, 0.0f)))
					m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}
			else
			{
				ImGui::BeginDisabled();
				ImGui::Button("<", ImVec2(28.0f, 0.0f));
				ImGui::EndDisabled();
			}
			ImGui::PopStyleColor();

			ImGui::SameLine();

			std::string rootLabel = m_RootDirectory.filename().string();
			if (rootLabel.empty())
				rootLabel = "Assets";

			if (ImGui::SmallButton(rootLabel.c_str()))
				m_CurrentDirectory = m_RootDirectory;

			std::error_code relativeError;
			std::filesystem::path relativePath = std::filesystem::relative(m_CurrentDirectory, m_RootDirectory, relativeError);
			std::filesystem::path breadcrumbPath = m_RootDirectory;
			if (!relativeError)
			{
				for (const auto& part : relativePath)
				{
					if (part == ".")
						continue;

					breadcrumbPath /= part;
					ImGui::SameLine();
					ImGui::TextDisabled("/");
					ImGui::SameLine();

					std::string label = part.string();
					ImGui::PushID(breadcrumbPath.string().c_str());
					if (ImGui::SmallButton(label.c_str()))
						m_CurrentDirectory = breadcrumbPath;
					ImGui::PopID();
				}
			}

			ImGui::TableSetColumnIndex(1);

			if (ImGui::Button("+ Folder"))
			{
				m_NewDirectoryName = "NewFolder";
				m_ShowCreateDirectoryPopup = true;
			}

			ImGui::SameLine();
			ImGui::SetNextItemWidth(searchWidth);
			ImGui::InputTextWithHint("##AssetSearch", "Search assets", m_SearchBuffer.data(), m_SearchBuffer.size());

			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Tile Size");
			ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
			ImGui::SetNextItemWidth(sliderWidth);
			ImGui::SliderInt("##TileSize", &m_IconSize, 48, 192);

			ImGui::EndTable();
		}

		ImGui::PopStyleVar();

		ImGui::Separator();
	}

	void AssetManagerPanel::RenderFolderHierarchy()
	{
		ImGui::TextDisabled("Folders");
		ImGui::Separator();

		if (!std::filesystem::exists(m_RootDirectory))
		{
			ImGui::TextDisabled("Missing asset root");
			return;
		}

		RenderDirectoryTreeNode(m_RootDirectory, true);
	}

	void AssetManagerPanel::RenderDirectoryTreeNode(const std::filesystem::path& directory, bool rootNode)
	{
		std::vector<std::filesystem::directory_entry> childDirectories;
		for (const auto& entry : GetVisibleEntries(directory, false))
		{
			if (entry.is_directory())
				childDirectories.push_back(entry);
		}

		std::string label = rootNode ? m_RootDirectory.filename().string() : directory.filename().string();
		if (label.empty())
			label = "Assets";

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (PathsEqual(directory, m_CurrentDirectory))
			flags |= ImGuiTreeNodeFlags_Selected;
		if (rootNode)
			flags |= ImGuiTreeNodeFlags_DefaultOpen;
		if (childDirectories.empty())
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

		ImGui::PushID(directory.string().c_str());
		bool open = ImGui::TreeNodeEx("##Folder", flags, "%s", label.c_str());
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			m_CurrentDirectory = directory;

		if (open && !childDirectories.empty())
		{
			for (const auto& childDirectory : childDirectories)
				RenderDirectoryTreeNode(childDirectory.path());

			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	bool AssetManagerPanel::ShouldHideEntry(const std::filesystem::directory_entry& entry) const
	{
		std::string fileNameStr = entry.path().filename().string();

		if (std::find(m_HiddenFiles.begin(), m_HiddenFiles.end(), fileNameStr) != m_HiddenFiles.end())
			return true;

		return std::any_of(m_HiddenFiles.begin(), m_HiddenFiles.end(), [&](const std::string& hiddenPattern) {
			if (hiddenPattern.find('*') != std::string::npos)
			{
				std::string pattern = std::regex_replace(hiddenPattern, std::regex("\\*"), ".*");
				return std::regex_match(fileNameStr, std::regex(pattern));
			}
			return false;
		});
	}

	bool AssetManagerPanel::EntryMatchesSearch(const std::filesystem::directory_entry& entry) const
	{
		if (m_SearchBuffer[0] == '\0')
			return true;

		std::string fileName = ToLowerCopy(entry.path().filename().string());
		std::string search = ToLowerCopy(m_SearchBuffer.data());
		return fileName.find(search) != std::string::npos;
	}

	std::vector<std::filesystem::directory_entry> AssetManagerPanel::GetVisibleEntries(const std::filesystem::path& directory, bool applySearch) const
	{
		std::vector<std::filesystem::directory_entry> entries;
		std::error_code ec;
		std::filesystem::directory_iterator it(directory, ec);
		if (ec)
			return entries;

		for (std::filesystem::directory_iterator end; it != end; it.increment(ec))
		{
			if (ec)
				break;

			const auto& entry = *it;
			if (ShouldHideEntry(entry))
				continue;
			if (applySearch && !EntryMatchesSearch(entry))
				continue;

			entries.push_back(entry);
		}

		std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
			bool lhsDirectory = lhs.is_directory();
			bool rhsDirectory = rhs.is_directory();
			if (lhsDirectory != rhsDirectory)
				return lhsDirectory > rhsDirectory;

			return ToLowerCopy(lhs.path().filename().string()) < ToLowerCopy(rhs.path().filename().string());
		});

		return entries;
	}

	void AssetManagerPanel::RenderDirectoryContents()
	{
		if (!std::filesystem::exists(m_CurrentDirectory))
		{
			ImGui::TextDisabled("Current folder no longer exists.");
			m_CurrentDirectory = m_RootDirectory;
			return;
		}

		ImRect dropRect(ImGui::GetCurrentWindow()->InnerRect.Min, ImGui::GetCurrentWindow()->InnerRect.Max);
		if (ImGui::GetDragDropPayload())
			ImGui::GetWindowDrawList()->AddRect(dropRect.Min, dropRect.Max, ImGui::GetColorU32(ImGuiCol_DragDropTarget), 4.0f, 0, 2.0f);

		auto entries = GetVisibleEntries(m_CurrentDirectory, true);
		if (entries.empty())
		{
			ImGui::TextDisabled(m_SearchBuffer[0] == '\0' ? "This folder is empty." : "No assets match the search.");
			HandleCurrentDirectoryDropTarget(dropRect);
			return;
		}

		ImGuiStyle& style = ImGui::GetStyle();
		float iconSize = static_cast<float>(m_IconSize);
		float tileWidth = std::max(iconSize + 24.0f, 96.0f);
		float tileHeight = iconSize + ImGui::GetTextLineHeight() + style.ItemSpacing.y + 10.0f;
		float panelWidth = ImGui::GetContentRegionAvail().x;
		int numColumns = std::max(static_cast<int>(panelWidth / (tileWidth + style.ItemSpacing.x)), 1);

		if (ImGui::BeginTable("AssetBrowserTable", numColumns, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadInnerX))
		{
			for (int column = 0; column < numColumns; column++)
				ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, tileWidth);

			for (const auto& entry : entries)
			{
				std::string filePath = entry.path().string();
				std::string fileNameStr = entry.path().filename().string();

				ImGui::TableNextColumn();
				ImGui::PushID(filePath.c_str());

				float cellStartX = ImGui::GetCursorPosX();
				float cellStartY = ImGui::GetCursorPosY();
				float iconOffsetX = (tileWidth - iconSize) * 0.5f;
				ImGui::SetCursorPosX(cellStartX + iconOffsetX);

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 0.5f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));

				if (entry.is_directory())
					RenderDirectoryEntry(entry);
				else
					RenderFileEntry(entry);

				ImGui::PopStyleColor(3);

				ImGui::SetCursorPosX(cellStartX);
				RenderEntryLabel(fileNameStr, tileWidth);

				float usedHeight = ImGui::GetCursorPosY() - cellStartY;
				if (usedHeight < tileHeight)
					ImGui::Dummy(ImVec2(1.0f, tileHeight - usedHeight));

				ImGui::PopID();
			}

			ImGui::EndTable();
		}

		HandleCurrentDirectoryDropTarget(dropRect);
	}

	void AssetManagerPanel::HandleCurrentDirectoryDropTarget(const ImRect& dropRect)
	{
		if (dropRect.GetWidth() <= 0.0f || dropRect.GetHeight() <= 0.0f)
			return;

		if (ImGui::BeginDragDropTargetCustom(dropRect, ImGui::GetID("##CurrentDirectoryDropTarget")))
		{
			std::string prefabPayload = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetPrefab);
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(prefabPayload.c_str()))
			{
				std::string filePath = std::string((char*)payload->Data, payload->DataSize > 0 ? payload->DataSize - 1 : 0);
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
					Application::Instance().GetAssetManager().Load<Prefab>(destPath.string(), false);
				}
			}

			std::string entityPayload = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::SceneEntity);
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(entityPayload.c_str()))
			{
				UUID entityUUID = *(const UUID*)payload->Data;

				if (m_Context && m_Context->ActiveScene())
				{
					Entity entity = m_Context->ActiveScene()->GetEntity(entityUUID);
					if (entity != Constants::Entities::InvalidEntityID)
					{
						auto& assetManager = Application::Instance().GetAssetManager();
						std::string filePath = (m_CurrentDirectory / (entity.GetName() + ".ebprefab")).string();

						// If prefab exists, remove existing prefab and create a new one in asset manager (but keep existing UUID)
						//if (assetManager.ContainsAssetWithPath(filePath))
						//{
						//	SharedPtr<Prefab> existingPrefab = assetManager.Load<Prefab>(filePath, false);
						//	if (existingPrefab)
						//		assetManager.RemoveAsset(existingPrefab->GetFilePath());
						//}

						SharedPtr<Prefab> prefab = m_Context->ActiveScene()->CreatePrefab(entity, filePath);
						if (prefab == nullptr)
						{
							auto evt = UINotificationEvent(std::format("Failed to create prefab from entity {}!", entity.GetName()), UINotificationEvent::Error);
							m_Context->EventCallback(evt);
							ImGui::EndDragDropTarget();
							return;
						}

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

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			switch (AssetPayloadTypeForPath(filePath))
			{
			case DragDropPayloadType::Scene:
				m_Context->RequestedSceneOpenPath = filePath.string();
				break;
			case DragDropPayloadType::AssetPrefab:
				m_Context->RequestedPrefabOpenPath = filePath.string();
				break;
			default:
				break;
			}
		}

		if (Application::Instance().GetAssetManager().ContainsAssetWithPath(filePath.string()))
		{
			ImVec2 imageMin = ImGui::GetItemRectMin();
			ImVec2 imageMax = ImGui::GetItemRectMax();
			float badgeSize = std::clamp(static_cast<float>(m_IconSize) * 0.22f, 16.0f, 22.0f);
			ImVec2 badgeMin(imageMax.x - badgeSize - 4.0f, imageMin.y + 4.0f);
			ImVec2 badgeMax(imageMax.x - 4.0f, imageMin.y + 4.0f + badgeSize);

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(badgeMin, badgeMax, ImGui::GetColorU32(ImVec4(0.15f, 0.55f, 0.95f, 0.95f)), 4.0f);
			drawList->AddRect(badgeMin, badgeMax, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.35f)), 4.0f);

			const char* badgeText = "A";
			ImVec2 textSize = ImGui::CalcTextSize(badgeText);
			ImVec2 textPos(badgeMin.x + (badgeSize - textSize.x) * 0.5f, badgeMin.y + (badgeSize - textSize.y) * 0.5f);
			drawList->AddText(textPos, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), badgeText);
		}
		
		RenderFileEntryContextMenu(entry);

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			auto filePathAbs = std::filesystem::absolute(filePath);
			auto payloadType = AssetPayloadTypeForPath(filePathAbs);
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

		// Context Mennu
		RenderDirectoryEntryContextMenu(entry);

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			auto filePathAbs = std::filesystem::absolute(entry);

			ImGui::SetDragDropPayload(DragDropDirectory, filePathAbs.string().c_str(), filePathAbs.string().size() + 1);
			ImGui::Image(m_DirectoryTexID, ImVec2(64, 64), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
			ImGui::EndDragDropSource();
		}

		// Accept all file-based drag drop payload types as drop targets for moving files into the directory
		if (ImGui::BeginDragDropTarget())
		{
			auto& assetManager = Application::Instance().GetAssetManager();

			// Check file types
			for (int i = 0; i < std::to_underlying(DragDropPayloadType::Count); i++)
			{
				auto payloadType = static_cast<DragDropPayloadType>(i);
				if (payloadType == DragDropPayloadType::None || payloadType == DragDropPayloadType::SceneEntity)
					continue;

				std::string payloadStr = DragDropUtils::DragDropPayloadTypeToString(payloadType);
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadStr.c_str()))
				{
					std::string filePath = std::string((char*)payload->Data, payload->DataSize - 1);
					std::filesystem::path srcPath(filePath);
					std::filesystem::path destPath = entry.path() / srcPath.filename();

					// Update the asset's filepath in the AssetManager if it's a tracked asset
					if (assetManager.ContainsAssetWithPath(filePath))
					{
						auto asset = assetManager.GetAssetByPath<Asset>(filePath);
						std::error_code ec;
						std::filesystem::rename(srcPath, destPath, ec);
						if (ec)
						{
							EB_CORE_ERROR("Failed to move asset to '{0}': {1}", destPath.string(), ec.message());
						}
						else
						{
							assetManager.RenameAsset(asset->GetUUID(), destPath.stem().string(), destPath.string());
						}
					}
					else
					{
						std::error_code ec;
						std::filesystem::rename(srcPath, destPath, ec);
						if (ec)
						{
							EB_CORE_ERROR("Failed to move file to '{0}': {1}", destPath.string(), ec.message());
						}
						else
						{
							EB_CORE_INFO("Successfully moved file to: {0}", destPath.string());
						}
					}

					break;
				}
			}

			// Check if it's a directory being moved
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(DragDropDirectory))
			{
				std::string filePath = std::string((char*)payload->Data, payload->DataSize - 1);
				std::filesystem::path destPath = entry.path() / std::filesystem::path(filePath).filename();

				// Create the destination directory if it doesn't exist
				if (!std::filesystem::exists(destPath))
				{
					std::error_code ec;
					std::filesystem::create_directory(destPath, ec);
					if (ec)
					{
						EB_CORE_ERROR("Failed to create directory at '{0}': {1}", destPath.string(), ec.message());
						return;
					}
				}

				// Move directory and update all assets within that directory in the AssetManager
				for (const auto& innerEntry : std::filesystem::recursive_directory_iterator(filePath))
				{
					auto filePathDiff = std::filesystem::relative(innerEntry.path(), filePath);
					auto destFilePath = destPath / filePathDiff;

					// Recreate subdirectory structure at the destination
					if (innerEntry.is_directory())
					{
						std::error_code ec;
						std::filesystem::create_directory(destFilePath, ec);
						continue;
					}

					if (assetManager.ContainsAssetWithPath(innerEntry.path().string()))
					{
						auto asset = assetManager.GetAssetByPath<Asset>(innerEntry.path().string());
						std::error_code ec;
						std::filesystem::rename(innerEntry.path(), destFilePath, ec);
						if (ec)
						{
							EB_CORE_ERROR("Failed to move asset to '{0}': {1}", destFilePath.string(), ec.message());
						}
						else
						{
							assetManager.RenameAsset(asset->GetUUID(), destFilePath.stem().string(), destFilePath.string());
						}
					}
					else
					{
						std::error_code ec;
						std::filesystem::rename(innerEntry.path(), destFilePath, ec);
						if (ec)
						{
							EB_CORE_ERROR("Failed to move file to '{0}': {1}", destFilePath.string(), ec.message());
						}
						else
						{
							EB_CORE_INFO("Successfully moved file to: {0}", destFilePath.string());
						}
					}
				}

				// Remove start directory after moving all contents
				std::error_code ec;
				std::filesystem::remove_all(filePath, ec);
			}

			ImGui::EndDragDropTarget();
		}
	}

	void AssetManagerPanel::RenderEntryLabel(const std::string& label, float tileWidth)
	{
		ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
		ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
		float visibleTextWidth = std::min(textSize.x, tileWidth);
		float textX = cursorScreenPos.x + std::max((tileWidth - visibleTextWidth) * 0.5f, 0.0f);
		ImVec2 textMin(textX, cursorScreenPos.y);
		ImVec2 textMax(cursorScreenPos.x + tileWidth, cursorScreenPos.y + ImGui::GetTextLineHeight());

		ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), textMin, textMax, textMax.x, label.c_str(), nullptr, &textSize);
		ImGui::Dummy(ImVec2(tileWidth, ImGui::GetTextLineHeight()));

		if (ImGui::IsItemHovered() && textSize.x > tileWidth)
			ImGui::SetTooltip("%s", label.c_str());
	}

	void AssetManagerPanel::RenderFileEntryContextMenu(const std::filesystem::directory_entry& entry)
	{
		const std::filesystem::path filePath = entry.path();
		const std::filesystem::path fileName = entry.path().filename();

		if (ImGui::BeginPopupContextItem())
		{
			auto& assetManager = Application::Instance().GetAssetManager();
			bool canAddAsAsset = !assetManager.ContainsAssetWithPath(filePath.string()) && IsValidAssetFile(filePath);
			if (canAddAsAsset)
			{
				if (ImGui::MenuItem("Add as Asset"))
				{
					SharedPtr<Asset> asset = AddFileAsAsset(filePath);
					if (asset)
					{
						auto evt = UINotificationEvent(std::format("Added asset: {}", asset->GetName()));
						m_Context->EventCallback(evt);
					}
					else
					{
						auto evt = UINotificationEvent(std::format("Failed to add asset: {}", fileName.string()), UINotificationEvent::Error);
						m_Context->EventCallback(evt);
					}
				}

				ImGui::Separator();
			}

			// Render asset-specific options based on the file extension
			auto assetType = AssetPayloadTypeForPath(filePath);
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
			case DragDropPayloadType::AssetSkeletonMask:
				RenderSkeletonMaskOptions(filePath.string());
				break;
			}

			// Common options for all asset types
			if (ImGui::MenuItem("Delete"))
			{
				m_PendingDeletePath = filePath;
				m_PendingDeleteIsDirectory = false;
				m_ShowDeleteConfirmPopup = true;
			}

			ImGui::EndPopup();
		}
	}

	void AssetManagerPanel::RenderDirectoryEntryContextMenu(const std::filesystem::directory_entry& entry)
	{
		const std::filesystem::path filePath = entry.path();
		const std::filesystem::path fileName = entry.path().filename();

		if (ImGui::BeginPopupContextItem())
		{
			// Common options for all asset types
			if (ImGui::MenuItem("Delete"))
			{
				m_PendingDeletePath = filePath;
				m_PendingDeleteIsDirectory = true;
				m_ShowDeleteConfirmPopup = true;
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

			if (ImGui::BeginMenu("New Asset"))
			{
				if (ImGui::MenuItem("Skeleton Mask"))
				{
					m_ShowSkeletonMaskOptionsPopup = true;
					m_SkeletonMaskName = "NewSkeletonMask";
					m_SkeletonPath.clear();
				}

				ImGui::EndMenu();
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
						asset = Application::Instance().GetAssetManager().Load<Prefab>(file);
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

	bool AssetManagerPanel::IsValidAssetFile(const std::filesystem::path& filePath) const
	{
		return AssetPayloadTypeForPath(filePath) != DragDropPayloadType::None;
	}

	SharedPtr<Asset> AssetManagerPanel::AddFileAsAsset(const std::filesystem::path& filePath)
	{
		auto& assetManager = Application::Instance().GetAssetManager();
		std::string path = filePath.string();
		SharedPtr<Asset> asset = nullptr;

		switch (AssetPayloadTypeForPath(filePath))
		{
		case DragDropPayloadType::AssetTexture:
			asset = assetManager.Load<Texture2D>(path, false);
			break;
		case DragDropPayloadType::AssetMesh:
			asset = assetManager.Load<Mesh>(path, false);
			break;
		case DragDropPayloadType::AssetModel:
			asset = assetManager.Load<Model>(path, false);
			break;
		case DragDropPayloadType::AssetScript:
			asset = assetManager.Load<Script>(path, false);
			break;
		case DragDropPayloadType::AssetShader:
			asset = assetManager.Load<Shader>(path, false);
			break;
		case DragDropPayloadType::AssetMaterial:
			asset = assetManager.Load<MaterialBase>(path, false);
			break;
		case DragDropPayloadType::AssetPhysicsMaterial:
			asset = assetManager.Load<PhysicsMaterial>(path, false);
			break;
		case DragDropPayloadType::AssetPrefab:
			asset = assetManager.Load<Prefab>(path, false);
			break;
		case DragDropPayloadType::AssetFont:
			asset = assetManager.Load<Font>(path, false);
			break;
		case DragDropPayloadType::AssetAudioClip:
			asset = assetManager.Load<AudioClip>(path, false);
			break;
		case DragDropPayloadType::AssetSkeletonMask:
			asset = assetManager.Load<SkeletonMask>(path, false);
			break;
		case DragDropPayloadType::AssetNavMeshData:
			asset = assetManager.Load<NavigationMeshData>(path, false);
			break;
		case DragDropPayloadType::Scene:
			asset = assetManager.Load<Scene>(path, false);
			break;
		default:
			return nullptr;
		}

		if (asset)
		{
			asset->SetIsEngineAsset(false);
			if (auto project = ProjectManager::GetActive())
			{
				AssetRegistrySerializer serializer(&assetManager);
				serializer.Serialize(project->GetAssetsFilePath().string());
			}
		}

		return asset;
	}

	void AssetManagerPanel::RenderTextureOptions(const std::string& filePath)
	{

	}

	void AssetManagerPanel::RenderModelOptions(const std::string& filePath)
	{

	}

	void AssetManagerPanel::RenderPrefabOptions(const std::string& filePath)
	{
		if (ImGui::MenuItem("Open Prefab"))
		{
			m_Context->RequestedPrefabOpenPath = filePath;
			ImGui::CloseCurrentPopup();
		}

	}

	void AssetManagerPanel::RenderSceneOptions(const std::string& filePath)
	{
		if (ImGui::MenuItem("Open Scene"))
		{
			m_Context->RequestedSceneOpenPath = filePath;
			ImGui::CloseCurrentPopup();
		}

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

	void AssetManagerPanel::RenderDeleteConfirmPopup()
	{
		if (m_ShowDeleteConfirmPopup)
		{
			ImGui::OpenPopup("Confirm Delete");
			m_ShowDeleteConfirmPopup = false;
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(380, 130), ImGuiCond_Appearing);

		if (ImGui::BeginPopupModal("Confirm Delete", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize))
		{
			std::string itemName = m_PendingDeletePath.filename().string();
			if (m_PendingDeleteIsDirectory)
				ImGui::TextWrapped("Delete directory \"%s\" and all of its contents? This cannot be undone.", itemName.c_str());
			else
				ImGui::TextWrapped("Delete \"%s\"? This cannot be undone.", itemName.c_str());

			ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()));

			if (ImGui::Button("Delete", ImVec2(120, 0)))
			{
				auto& assetManager = Application::Instance().GetAssetManager();

				if (m_PendingDeleteIsDirectory)
				{
					for (const auto& entry : std::filesystem::recursive_directory_iterator(m_PendingDeletePath))
						assetManager.RemoveAsset(entry.path().string());

					std::error_code ec;
					std::filesystem::remove_all(m_PendingDeletePath, ec);
					if (ec)
					{
						auto evt = UINotificationEvent(std::format("Failed to delete directory '{}': {}", itemName, ec.message()), UINotificationEvent::Error);
						m_Context->EventCallback(evt);
					}
					else
					{
						auto evt = UINotificationEvent(std::format("Successfully deleted directory: {}", itemName));
						m_Context->EventCallback(evt);
					}
				}
				else
				{
					assetManager.RemoveAsset(m_PendingDeletePath.string());
					std::error_code ec;
					std::filesystem::remove(m_PendingDeletePath, ec);
					if (ec)
					{
						EB_CORE_ERROR("Failed to delete asset '{0}': {1}", itemName, ec.message());
					}
					else
					{
						EB_CORE_INFO("Successfully deleted asset: {0}", itemName);
					}
				}

				m_PendingDeletePath.clear();
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				m_PendingDeletePath.clear();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void AssetManagerPanel::RenderSkeletonMaskOptionsPopup()
	{
		if (m_ShowSkeletonMaskOptionsPopup)
		{
			ImGui::OpenPopup("Create Skeleton Mask");
			m_ShowSkeletonMaskOptionsPopup = false;
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500, 200), ImGuiCond_Appearing);

		if (ImGui::BeginPopupModal("Create Skeleton Mask", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize))
		{
			auto& assetManager = Application::Instance().GetAssetManager();
			SharedPtr<Skeleton> skeletonAsset = nullptr;
			std::string skeletonPreview = "None";
			if (!m_SkeletonPath.empty())
			{
				skeletonAsset = assetManager.GetAssetByPath<Skeleton>(m_SkeletonPath);
				if (skeletonAsset)
					skeletonPreview = skeletonAsset->GetName();
				else
					m_SkeletonPath.clear();
			}

			if (UI::PropertyGrid::Begin("SkeletonMaskOptions"))
			{
				UI::PropertyGrid::InputText("Mask Name", m_SkeletonMaskName);

				auto skeletonAssets = assetManager.GetAssetsOfType<Skeleton>();
				if (UI::PropertyGrid::BeginComboBox("Skeleton", skeletonPreview))
				{
					if (UI::PropertyGrid::ComboBoxItem("None", m_SkeletonPath.empty()))
					{
						m_SkeletonPath.clear();
						skeletonAsset = nullptr;
					}

					for (size_t i = 0; i < skeletonAssets.size(); i++)
					{
						bool isSelected = skeletonAsset == skeletonAssets[i];
						if (UI::PropertyGrid::ComboBoxItem(skeletonAssets[i]->GetName(), isSelected))
						{
							skeletonAsset = skeletonAssets[i];
							m_SkeletonPath = skeletonAsset->GetFilePath();
							skeletonPreview = skeletonAsset->GetName();
						}
					}

					UI::PropertyGrid::EndComboBox();
				}
				UI::PropertyGrid::End();
			}

			ImGui::BeginDisabled(!skeletonAsset || m_SkeletonMaskName.empty());
			if (ImGui::Button("Create", ImVec2(120, 0)))
			{
				auto skeletonMaskPath = m_CurrentDirectory / (m_SkeletonMaskName + ".ebmask");
				auto skeletonMask = SharedPtr<SkeletonMask>::Create(m_SkeletonMaskName);
				skeletonMask->SetFilePath(skeletonMaskPath.string());
				skeletonMask->SetSkeleton(skeletonAsset);
				skeletonMask->SetIsEngineAsset(false);

				SkeletonMaskSerializer serializer;
				if (serializer.Serialize(skeletonMaskPath, skeletonMask))
				{
					// Open the new skeleton mask in the editor
					m_Context->RequestSkeletonMaskOpenPath = skeletonMaskPath.string();
					assetManager.Load<SkeletonMask>(skeletonMaskPath.string());
				}
				else
				{
					auto evt = UINotificationEvent(std::format("Failed to create skeleton mask asset '{}'", m_SkeletonMaskName), UINotificationEvent::Error);
					m_Context->EventCallback(evt);
					assetManager.RemoveAsset(skeletonMask->GetUUID());
				}

				ImGui::CloseCurrentPopup();
			}
			ImGui::EndDisabled();
			ImGui::SameLine();

			if (ImGui::Button("Cancel", ImVec2(120, 0)))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void AssetManagerPanel::RenderAudioClipOptions(const std::string& filePath)
	{
		auto& assetManager = Application::Instance().GetAssetManager();
		if (!assetManager.ContainsAssetWithPath(filePath))
			return;

		auto audioClip = assetManager.GetAssetByPath<AudioClip>(filePath);
		if (!audioClip)
			return;

		if (ImGui::BeginMenu("Load Mode"))
		{
			auto loadModeItem = [&](const char* label, AudioLoadMode mode, const char* tooltip)
			{
				if (ImGui::MenuItem(label, nullptr, audioClip->GetLoadMode() == mode))
				{
					audioClip->SetLoadMode(mode);
					if (auto project = ProjectManager::GetActive())
					{
						AssetRegistrySerializer serializer(&assetManager);
						serializer.Serialize(project->GetAssetsFilePath().string());
					}
				}

				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", tooltip);
			};

			loadModeItem("Auto", AudioLoadMode::Auto, "Stream large files, decode small ones.");
			loadModeItem("Decode", AudioLoadMode::Decode, "Decode into memory once and keep it cached.\nBest for short, frequently played sound effects.");
			loadModeItem("Stream", AudioLoadMode::Stream, "Decode on the fly straight off disk.\nBest for music and other long clips.");

			ImGui::EndMenu();
		}
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

	void AssetManagerPanel::RenderSkeletonMaskOptions(const std::string& filePath)
	{
		if (ImGui::MenuItem("Edit Skeleton Mask"))
		{
			m_Context->RequestSkeletonMaskOpenPath = filePath;
			ImGui::CloseCurrentPopup();
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