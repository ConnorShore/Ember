#pragma once

#include "ColliderComponentUI.h"
#include "UI/PropertyGrid.h"
#include "Ember/Math/Math.h"

#include <imgui/imgui.h>

namespace Ember {

	class CapsuleColliderComponentUI : public ColliderComponentUI<CapsuleColliderComponent>
	{
	public:
		CapsuleColliderComponentUI(EditorContext* context) : ColliderComponentUI(context) {}
		inline const char* GetName() const override { return "Capsule Collider"; }

	protected:
		inline void RenderComponentProperties(CapsuleColliderComponent& component) override
		{
			if (UI::PropertyGrid::Begin("CapsuleColliderProps"))
			{
				bool changed = false;
				if (UI::PropertyGrid::Float("Radius", component.Radius, 0.01f, 0.01f, 10000000.0f))
				{
					if (component.Radius < 0.01f)
						component.Radius = 0.01f;
					changed = true;
				}
				if (UI::PropertyGrid::Float("Height", component.Height, 0.01f, 0.01f, 10000000.0f))
				{
					if (component.Height < 0.01f)
						component.Height = 0.01f;
					changed = true;
				}
				changed |= UI::PropertyGrid::Checkbox("Is Trigger", component.IsTrigger);
				if (changed)
					component.NeedsRebuild = true;

				UI::PropertyGrid::End();
			}
		}
	};

}