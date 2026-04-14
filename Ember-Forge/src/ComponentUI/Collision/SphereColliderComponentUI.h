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
				UI::PropertyGrid::Float("Radius", component.Radius);
				UI::PropertyGrid::Float3("Offset", component.Offset);

				UI::PropertyGrid::End();
			}
		}
	};
}
