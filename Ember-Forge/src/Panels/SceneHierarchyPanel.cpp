#include "SceneHierarchyPanel.h"

namespace Ember {

	SceneHierarchyPanel::SceneHierarchyPanel()
		: Panel("Scene Hierarchy")
	{
	}

	SceneHierarchyPanel::~SceneHierarchyPanel()
	{
	}

	void SceneHierarchyPanel::OnEvent(Event& event)
	{
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::Begin(m_Title.c_str());

		ImGui::Button("Create Empty Entity");

		ImGui::End();
	}

}