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
		bool SupportsFitToMesh() const override { return true; }

		void FitColliderToBounds(BoxColliderComponent& component, const Vector3f& localMin, const Vector3f& localMax) override
		{
			// Size is full extents, and the offset re-centres the box on a mesh whose origin is not
			// at its middle - a wall pivoted at its base, for example.
			component.Size = Math::Max(localMax - localMin, Vector3f(0.001f));
			component.Offset.Position = (localMin + localMax) * 0.5f;
		}

		virtual void RenderComponentProperties(BoxColliderComponent& component) override
		{
			if (UI::PropertyGrid::Begin("BoxColliderProps"))
			{
				bool changed = false;
				changed |= UI::PropertyGrid::Float3("Size", component.Size);
				changed |= UI::PropertyGrid::Checkbox("Is Trigger", component.IsTrigger);

				if (changed)
					component.NeedsRebuild = true;

				UI::PropertyGrid::End();
			}
		}
	};
}