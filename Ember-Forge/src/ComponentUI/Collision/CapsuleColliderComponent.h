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
				UI::PropertyGrid::Float("Radius", component.Radius);
				UI::PropertyGrid::Float("Height", component.Height);
				UI::PropertyGrid::Float3("Offset", component.Offset);

				UI::PropertyGrid::End();
			}
		}
	};

}