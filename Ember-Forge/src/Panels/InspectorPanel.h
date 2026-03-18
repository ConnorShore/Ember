#pragma once

#include "Panel.h"

namespace Ember {

	class InspectorPanel : public Panel
	{
	public:
		InspectorPanel();
		virtual ~InspectorPanel();

		void OnEvent(Event& event) override;
		void OnImGuiRender() override;

	private:
		Entity m_SelectedEntity;
	};
}