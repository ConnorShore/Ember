#include "efpch.h"
#include "AnimationInspectorPanel.h"

namespace Ember {

	AnimationInspectorPanel::AnimationInspectorPanel(EditorContext* context)
		: InspectorPanelContent(context)
	{
	}

	AnimationInspectorPanel::~AnimationInspectorPanel()
	{
	}

	void AnimationInspectorPanel::OnImGuiRender()
	{
		ImGui::Text("Animation Inspector Panel");
	}

}