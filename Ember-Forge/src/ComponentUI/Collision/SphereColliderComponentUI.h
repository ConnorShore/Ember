#pragma once

#include "ColliderComponentUI.h"

#include "UI/PropertyGrid.h"
#include "Ember/Math/Math.h"

#include <imgui/imgui.h>

namespace Ember {

	class SphereColliderComponentUI : public ColliderComponentUI<SphereColliderComponent>
	{
	public:
		SphereColliderComponentUI(EditorContext* context) : ColliderComponentUI(context) {}
		inline const char* GetName() const override { return "Sphere Collider"; }

	protected:
		virtual void RenderComponentProperties(SphereColliderComponent& component) override
		{
			if (UI::PropertyGrid::Begin("SphereColliderProps"))
			{
				bool changed = false;
				changed |= UI::PropertyGrid::Float("Radius", component.Radius);
				changed |= UI::PropertyGrid::Float3("Offset Position", component.Offset.Position);
				changed |= UI::PropertyGrid::Float3("Offset Rotation", component.Offset.Rotation);
				if (changed)
					component.NeedsRebuild = true;

				UI::PropertyGrid::End();
			}
		}
	};
}
