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

	private:
		std::filesystem::path m_AssetDirectory, m_CurrentDirectory;
		ImTextureID m_FileTexID, m_DirectoryTexID;
	};
}