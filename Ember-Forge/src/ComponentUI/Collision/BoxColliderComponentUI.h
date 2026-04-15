#pragma once

#include "ColliderComponentUI.h"

#include "UI/PropertyGrid.h"
#include "Ember/Math/Math.h"

#include <imgui/imgui.h>

namespace Ember {

	class BoxColliderComponentUI : public ColliderComponentUI<BoxColliderComponent>
	{
	public:
		BoxColliderComponentUI(EditorContext* context) : ColliderComponentUI(context) {}
		inline const char* GetName() const override { return "Box Collider"; }

	protected:
		virtual void RenderComponentProperties(BoxColliderComponent& component) override
		{
			if (UI::PropertyGrid::Begin("BoxColliderProps"))
			{
				bool changed = false;
				changed |= UI::PropertyGrid::Float3("Size", component.Size);
				changed |= UI::PropertyGrid::Float3("Offset Position", component.Offset.Position);
				changed |= UI::PropertyGrid::Float3("Offset Rotation", component.Offset.Rotation);
				if (changed)
					component.NeedsRebuild = true;

				UI::PropertyGrid::End();
			}
		}
	};
}