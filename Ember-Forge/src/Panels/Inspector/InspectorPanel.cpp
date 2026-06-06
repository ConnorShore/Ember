#include "efpch.h"
#include "InspectorPanel.h"
#include "SceneInspectorPanel.h"
#include "AnimationInspectorPanel.h"

#include <imgui/imgui.h>

#include <format>
#include <unordered_set>

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// InspectorPanel
	//////////////////////////////////////////////////////////////////////////

	InspectorPanel::InspectorPanel(EditorContext* context)
		: Panel("Inspector", context)
	{
		m_PanelContents[InspectorPanelState::Scene] = SharedPtr<SceneInspectorPanel>::Create(context);
		m_PanelContents[InspectorPanelState::Animation] = SharedPtr<AnimationInspectorPanel>::Create(context);
	}

	InspectorPanel::~InspectorPanel()
	{
	}

	void InspectorPanel::OnImGuiRender()
	{
		ImGui::Begin(m_Title.c_str());

		auto panelState = GetPanelState();
		if (panelState != InspectorPanelState::None)
			m_PanelContents[panelState]->OnImGuiRender();

		ImGui::End();
	}

	InspectorPanel::InspectorPanelState InspectorPanel::GetPanelState()
	{
		auto activeViewportType = m_Context->ActiveViewportViewer 
			? m_Context->ActiveViewportViewer->GetType()
			: EditorViewportViewer::Type::None;

		switch (activeViewportType)
		{
		case EditorViewportViewer::Type::Scene:
			return InspectorPanelState::Scene;
		case EditorViewportViewer::Type::Prefab:
			return InspectorPanelState::Scene;	// Prefabs use the same inspector as scenes
		case EditorViewportViewer::Type::Animation:
			return InspectorPanelState::Animation;
		case EditorViewportViewer::Type::None:
		default:
			return InspectorPanelState::None;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// InspectorPanelContext
	//////////////////////////////////////////////////////////////////////////

	InspectorPanelContent::InspectorPanelContent(EditorContext* context)
		: m_Context(context)
	{
	}

	InspectorPanelContent::~InspectorPanelContent()
	{
	}

}