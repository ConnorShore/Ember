#pragma once

#include "Panels/Panel.h"

#include "ComponentUI/ComponentUI.h"

#include <unordered_map>

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Inspector Panel Content
	///////////////////////////////////////////////////////////////////////////

	class InspectorPanelContent : public SharedResource
	{
	public:
		InspectorPanelContent(EditorContext* context);
		virtual ~InspectorPanelContent();

		virtual void OnImGuiRender() = 0;

	protected:
		EditorContext* m_Context;
	};

	//////////////////////////////////////////////////////////////////////////
	// Inspector Panel
	//////////////////////////////////////////////////////////////////////////

	class InspectorPanel : public Panel
	{
	public:
		InspectorPanel(EditorContext* context);
		virtual ~InspectorPanel();

		virtual void OnImGuiRender() override;

	public:
		enum class InspectorPanelState
		{
			None = 0,
			Scene,
			Animation
		};

	private:
		InspectorPanelState GetPanelState();

	private:
		std::unordered_map<InspectorPanelState, SharedPtr<InspectorPanelContent>> m_PanelContents;
	};
}