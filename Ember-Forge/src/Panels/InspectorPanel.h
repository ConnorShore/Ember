#pragma once

#include "Panel.h"

#include "ComponentUI/ComponentUI.h"

#include <vector>

namespace Ember {

	class InspectorPanel : public Panel
	{
	public:
		InspectorPanel();
		virtual ~InspectorPanel();

		void OnEvent(Event& event) override;
		void OnImGuiRender() override;

	private:
		void DrawEntityHeader(Entity entity);

	private:
		std::vector<ScopedPtr<ComponentUIBase>> m_ComponentUIs;
	};
}