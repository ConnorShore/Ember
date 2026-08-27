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
		bool SupportsFitToMesh() const override { return true; }

		void FitColliderToBounds(SphereColliderComponent& component, const Vector3f& localMin, const Vector3f& localMax) override
		{
			// The bounding sphere, so the mesh is fully contained rather than clipped at its corners.
			Vector3f halfExtents = (localMax - localMin) * 0.5f;
			component.Radius = Math::Max(Math::Length(halfExtents), 0.001f);
			component.Offset.Position = (localMin + localMax) * 0.5f;
		}

		virtual void RenderComponentProperties(SphereColliderComponent& component) override
		{
			if (UI::PropertyGrid::Begin("SphereColliderProps"))
			{
				bool changed = false;
				changed |= UI::PropertyGrid::Float("Radius", component.Radius);
				changed |= UI::PropertyGrid::Checkbox("Is Trigger", component.IsTrigger);
				if (changed)
					component.NeedsRebuild = true;

				UI::PropertyGrid::End();
			}
		}
	};
}
