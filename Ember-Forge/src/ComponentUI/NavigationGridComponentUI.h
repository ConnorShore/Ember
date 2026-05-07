#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"

#include <Ember/AI/NavigationGrid.h>

#include <imgui/imgui.h>

namespace Ember {

	class NavigationGridComponentUI : public ComponentUI<NavigationGridComponent>
	{
	public:
		NavigationGridComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Navigation Grid Component"; }

	protected:
		inline void RenderComponentImpl(NavigationGridComponent& component) override
		{
			if (UI::PropertyGrid::Begin("NavigationGridProps"))
			{
				UI::PropertyGrid::Float("Node Spacing", component.NodeSpacing, 0.01f, 0.1f);

				// Generate (or Regenerate) Grid Button
				std::string buttonText = component.Generated ? "Re-Generate Grid" : "Generate Grid";
				if (ImGui::Button(buttonText.c_str()))
				{
					auto& transform = m_Context->SelectedEntity.GetComponent<TransformComponent>();
					Vector3f entityPos = transform.WorldTransform[3];
					Vector2f size = { transform.Scale.x, transform.Scale.z };

					component.Grid = NavigationGrid::Generate(entityPos, size.x, size.y, component.NodeSpacing);
					component.Generated = true;
				}

				UI::PropertyGrid::End();
			}
		}
	};

}