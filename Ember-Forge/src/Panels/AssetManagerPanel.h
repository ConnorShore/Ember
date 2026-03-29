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

		void UpdateAssetDirectory(const std::filesystem::path& newDirectory);

	private:
		std::string SelectAndLoadFile(const std::string& name, const std::string& type);

	private:
		std::filesystem::path m_AssetDirectory, m_CurrentDirectory;
		ImTextureID m_FileTexID, m_DirectoryTexID;

		int m_IconSize = 100;

		std::array<std::string, 1> m_HiddenFiles = { "Assets.eba" };
	};
}