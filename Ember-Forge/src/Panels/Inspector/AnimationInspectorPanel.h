#pragma once

#include "InspectorPanel.h"
#include "GraphNodeUI/GraphNodeUI.h"

namespace Ember {

	class AssetManager;

	class AnimationInspectorPanel : public InspectorPanelContent
	{
	public:
		AnimationInspectorPanel(EditorContext* context);
		virtual ~AnimationInspectorPanel();

		virtual void OnImGuiRender() override;

	private:
		void RenderAnimationState(AnimationState* animState);
		void RenderAnimationTransition(AnimationTransition* animTransition);

	private:
		AssetManager& m_AssetManager;
	};

}