#include "InspectorPanel.h"

#include <Ember.h>

namespace Ember {

	InspectorPanel::InspectorPanel()
		: Panel ("Inspector")
	{
	}

	InspectorPanel::~InspectorPanel()
	{
	}

	void InspectorPanel::OnEvent(Event& event)
	{
	}

	void InspectorPanel::OnImGuiRender()
	{
		if (m_SelectedEntity == InvalidEntityID)
		{
			// Blank panel if no entity selected
			ImGui::Begin(m_Title.c_str());
			ImGui::End();
			return;
		}

		ImGui::Begin(m_Title.c_str());

		ImGui::Button("Add Component");

		ImGui::End();
	}

}