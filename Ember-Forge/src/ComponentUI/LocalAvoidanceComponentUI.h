#pragma once

#include "ComponentUI.h"
#include "UI/PropertyGrid.h"
#include "UI/FilterWidget.h"

namespace Ember {

	class LocalAvoidanceComponentUI : public ComponentUI<LocalAvoidanceComponent>
	{
	public:
		LocalAvoidanceComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Local Avoidance Component"; }

	protected:
		inline void RenderComponentImpl(LocalAvoidanceComponent& component) override
		{
			if (UI::PropertyGrid::Begin("LocalAvoidanceProps"))
			{
				UI::PropertyGrid::Float("AvoidanceRadius", component.AvoidanceRadius);
				UI::PropertyGrid::Float("AvoidanceStrength", component.AvoidanceStrength);

				UI::PropertyGrid::FilterGrid("Avoidance Mask", component.AvoidanceMask, ProjectManager::GetActive()->GetCollisionFilterManager());

				UI::PropertyGrid::End();
			}
		}
	};

}