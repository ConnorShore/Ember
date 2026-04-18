#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"

namespace Ember {

	class LifetimeComponentUI : public ComponentUI<LifetimeComponent>
	{
	public:
		LifetimeComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Lifetime Component"; }

	protected:
		inline void RenderComponentImpl(LifetimeComponent& component) override
		{
			if (UI::PropertyGrid::Begin("LifetimeProps"))
			{
				UI::PropertyGrid::Float("Lifetime", component.Lifetime);

				UI::PropertyGrid::End();
			}
		}
	};

}