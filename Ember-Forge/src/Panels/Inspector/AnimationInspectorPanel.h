#pragma once

#include "InspectorPanel.h"

namespace Ember {

	class AnimationInspectorPanel : public InspectorPanelContent
	{
	public:
		AnimationInspectorPanel(EditorContext* context);
		virtual ~AnimationInspectorPanel();

		virtual void OnImGuiRender() override;

	private:

	};

}