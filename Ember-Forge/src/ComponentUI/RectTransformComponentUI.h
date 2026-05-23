#pragma once
#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"
#include "UI/DragDropTypes.h"

#include "Ember/Asset/Font.h"

#include <imgui/imgui.h>

namespace Ember {

	class RectTransformComponentUI : public ComponentUI<RectTransformComponent>
	{
	public:
		RectTransformComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Rect Transform Component"; }

	protected:
		inline void RenderComponentImpl(RectTransformComponent& component) override
		{
			if (UI::PropertyGrid::Begin("RectTransformProps"))
			{
				UI::PropertyGrid::Float2("Anchor Min", component.AnchorMin, 0.01f, 0.0f, 1.0f);
				UI::PropertyGrid::Float2("Anchor Max", component.AnchorMax, 0.01f, 0.0f, 1.0f);
				UI::PropertyGrid::Float2("Pivot", component.Pivot, 0.01f, 0.0f, 1.0f);
				UI::PropertyGrid::Float2("Size Delta", component.SizeDelta);
				UI::PropertyGrid::Float2("Anchored Position", component.AnchoredPosition);

				// Handle rotation in degrees for better UX
				float rotationDegrees = Math::Degrees(component.Rotation);
				if (UI::PropertyGrid::Float("Rotation", rotationDegrees, 0.1f, -360.0f, 360.0f))
					component.Rotation = Math::Radians(rotationDegrees);

				UI::PropertyGrid::End();
			}
		}

	};

}