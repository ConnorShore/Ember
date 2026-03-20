#include "AssetManagerPanel.h"

namespace Ember {

	AssetManagerPanel::AssetManagerPanel(EditorContext* context)
		: Panel("Asset Manager", context)
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