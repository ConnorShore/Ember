#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"

namespace Ember {

	class WaypointComponentUI : public ComponentUI<WaypointComponent>
	{
	public:
		WaypointComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Waypoint Component"; }

	protected:
		inline void RenderComponentImpl(WaypointComponent& component) override
		{
			if (UI::PropertyGrid::Begin("WaypointProps"))
			{
				UI::PropertyGrid::Checkbox("Show Paths", component.ShowPaths);
				UI::PropertyGrid::End();
			}
		}
	};

}