#pragma once

#include "Panel.h"
#include <array>
#include <filesystem>
#include <vector>


namespace Ember {

	class Asset;

	class AssetManagerPanel : public Panel
	{
	public:
		AssetManagerPanel(EditorContext* context);
		virtual ~AssetManagerPanel();

		void OnAttach() override;
		void OnImGuiRender() override;

		void UpdateRootDirectory(const std::filesystem::path& newDirectory);
		void UpdateCurrentDirectory(const std::filesystem::path& newDirectory) { m_CurrentDirectory = newDirectory; }

	private:
		void RenderPanelControls();
		void RenderFolderHierarchy();
		void RenderDirectoryTreeNode(const std::filesystem::path& directory, bool rootNode = false);
		void RenderDirectoryContents();
		void RenderFileEntry(const std::filesystem::directory_entry& entry);
		void RenderDirectoryEntry(const std::filesystem::directory_entry& entry);
		void RenderEntryLabel(const std::string& label, float tileWidth);
		void RenderFileEntryContextMenu(const std::filesystem::directory_entry& entry);
		void RenderDirectoryEntryContextMenu(const std::filesystem::directory_entry& entry);
		void RenderAssetPanelContextMenu();
		void HandleCurrentDirectoryDropTarget(const ImRect& dropRect);

		void RenderTextureOptions(const std::string& filePath);
		void RenderModelOptions(const std::string& filePath);
		void RenderPrefabOptions(const std::string& filePath);
		void RenderSceneOptions(const std::string& filePath);
		void RenderAudioClipOptions(const std::string& filePath);
		void RenderFontOptions(const std::string& filePath);
		void RenderShaderOptions(const std::string& filePath);
		void RenderScriptOptions(const std::string& filePath);

		void RenderRenameScenePopup();
		void RenameScene();

		void RenderCreateDirectoryPopup();
		void RenderDeleteConfirmPopup();

		std::string SelectAndLoadFile(const std::string& name, const std::string& type);
		SharedPtr<Asset> AddFileAsAsset(const std::filesystem::path& filePath);
		bool IsValidAssetFile(const std::filesystem::path& filePath) const;
		bool ShouldHideEntry(const std::filesystem::directory_entry& entry) const;
		bool EntryMatchesSearch(const std::filesystem::directory_entry& entry) const;
		std::vector<std::filesystem::directory_entry> GetVisibleEntries(const std::filesystem::path& directory, bool applySearch) const;

	private:
		std::filesystem::path m_RootDirectory, m_CurrentDirectory;
		ImTextureID m_FileTexID, m_DirectoryTexID;

		int m_IconSize = 100;
		float m_FolderPaneWidth = 220.0f;
		std::array<char, 128> m_SearchBuffer = {};

		std::array<std::string, 2> m_HiddenFiles = { "Assets.eba", "*.ebproj"};

		// Rename Scene dialog state
		bool m_ShowRenameScenePopup = false;
		std::string m_RenameSceneOldFilePath;
		std::string m_RenameSceneNewName;

		// Create directory popup
		bool m_ShowCreateDirectoryPopup = false;
		std::string m_NewDirectoryName;

		// Delete confirmation dialog state
		bool m_ShowDeleteConfirmPopup = false;
		bool m_PendingDeleteIsDirectory = false;
		std::filesystem::path m_PendingDeletePath;
	};
}