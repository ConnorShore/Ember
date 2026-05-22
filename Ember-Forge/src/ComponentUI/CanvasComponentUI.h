#pragma once
#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"
#include "UI/DragDropTypes.h"

#include "Ember/Asset/Font.h"

#include <imgui/imgui.h>

namespace Ember {

	class CanvasComponentUI : public ComponentUI<CanvasComponent>
	{
	public:
		CanvasComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Canvas Component"; }

	protected:
		inline void RenderComponentImpl(CanvasComponent& component) override
		{
			if (UI::PropertyGrid::Begin("CanvasProps"))
			{
				// Render Mode dropdown
				if (UI::PropertyGrid::BeginComboBox("Render Mode", RenderModeToString(component.RenderMode)))
				{
					for (int i = 0; i < 2; i++)
					{
						CanvasRenderMode mode = static_cast<CanvasRenderMode>(i);
						if (UI::PropertyGrid::ComboBoxItem(RenderModeToString(mode), component.RenderMode == mode))
						{
							component.RenderMode = mode;
							component.IsDirty = true;
						}
					}

					UI::PropertyGrid::EndComboBox();
				}

				// Canvas properties
				UI::PropertyGrid::Float2("Reference Resolution", component.ReferenceResolution);
				UI::PropertyGrid::Float("Match Width/Height", component.MatchWidthOrHeight, 0.01f, 0.0f, 1.0f);
				UI::PropertyGrid::Float("Plane Distance", component.PlaneDistance, 0.1f, 0.1f, 100.0f);
				UI::PropertyGrid::UInt("Sort Order", component.SortOrder);

				UI::PropertyGrid::End();
			}
		}

	private:
		std::string RenderModeToString(CanvasRenderMode mode)
		{
			switch (mode)
			{
			case CanvasRenderMode::ScreenSpace:
				return "Screen Space";
			case CanvasRenderMode::WorldSpace:
				return "World Space";
			default:
				return "Unknown";
			}
		}
	};

}