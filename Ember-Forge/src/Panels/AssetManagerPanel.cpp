#include "AssetManagerPanel.h"

namespace Ember {

	AssetManagerPanel::AssetManagerPanel()
		: Panel("Asset Manager")
	{
	}

	AssetManagerPanel::~AssetManagerPanel()
	{
	}

	void AssetManagerPanel::OnImGuiRender()
	{
		ImGui::Begin(m_Title.c_str());

		ImGui::End();
	}

}