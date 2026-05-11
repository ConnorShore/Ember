#pragma once

#include "Panel.h"
#include <filesystem>


namespace Ember {

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
		void RenderDirectoryContents();
		void RenderFileEntry(const std::filesystem::directory_entry& entry);
		void RenderDirectoryEntry(const std::filesystem::directory_entry& entry);
		void RenderFileEntryContextMenu(const std::filesystem::directory_entry& entry);
		void RenderAssetPanelContextMenu();

		std::string SelectAndLoadFile(const std::string& name, const std::string& type);

	private:
		std::filesystem::path m_RootDirectory, m_CurrentDirectory;
		ImTextureID m_FileTexID, m_DirectoryTexID;

		int m_IconSize = 100;

		std::array<std::string, 2> m_HiddenFiles = { "Assets.eba", "*.ebproj"};
	};
}