#include "efpch.h"
#include "AnimationInspectorPanel.h"
#include "Viewers/AnimationViewportViewer.h"

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
		auto animationViewport = static_cast<AnimationViewportViewer*>(m_Context->ActiveViewportViewer);
		
		auto selectedState = animationViewport->GetSelectedState();
		auto selectedTransition = animationViewport->GetSelectedTransition();
		if (!selectedState && !selectedTransition)
			return;

		if (selectedState)
			RenderAnimationState(selectedState);

		if (selectedTransition)
			RenderAnimationTransition(selectedTransition);
	}

	void AnimationInspectorPanel::RenderAnimationState(AnimationState* animState)
	{
		std::string message = "Selected state info: " + animState->Name;
		ImGui::Text(message.c_str());
	}

	void AnimationInspectorPanel::RenderAnimationTransition(AnimationTransition* animTransition)
	{
		std::string message = std::format("Selected transition from id {} to id {}", animTransition->FromStateId, animTransition->ToStateId);
		ImGui::Text(message.c_str());

		// TODO: If two way transition exists, show both directions as separate "expandable nodes" in the inspector with their own conditions and properties
	}

}