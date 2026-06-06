#pragma once

#include "InspectorPanel.h"
#include "GraphNodeUI/GraphNodeUI.h"

namespace Ember {

	class AnimationInspectorPanel : public InspectorPanelContent
	{
	public:
		AnimationInspectorPanel(EditorContext* context);
		virtual ~AnimationInspectorPanel();

		virtual void OnImGuiRender() override;

	private:
		void RenderAnimationState(AnimationState* animState);
		void RenderAnimationTransition(AnimationTransition* animTransition);
	};

}