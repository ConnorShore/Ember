#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"

namespace Ember {

	class AIPathComponentUI : public ComponentUI<AIPathComponent>
	{
	public:
		AIPathComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "A.I. Path Component"; }

	protected:
		inline void RenderComponentImpl(AIPathComponent& component) override
		{
			if (UI::PropertyGrid::Begin("AIPathProps"))
			{
				UI::PropertyGrid::Float("Speed", component.Speed);
				UI::PropertyGrid::Float("Arrival Tolerance", component.ArrivalTolerance, 0.01f, 0.0f, 100.0f);
				UI::PropertyGrid::End();
			}
		}
	};

}