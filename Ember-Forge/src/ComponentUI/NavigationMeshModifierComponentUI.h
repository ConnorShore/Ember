#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"

namespace Ember {

	class NavigationMeshModifierComponentUI : public ComponentUI<NavigationMeshModifierComponent>
	{
	public:
		NavigationMeshModifierComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Navigation Mesh Modifier Component"; }

	protected:
		inline void RenderComponentImpl(NavigationMeshModifierComponent& component) override
		{
			if (UI::PropertyGrid::Begin("NavigationMeshModifierProps"))
			{
				// Get all navigation mesh assets in the project to display in a combo box
				auto navMeshes = Application::Instance().GetAssetManager().GetAssetsOfType<NavigationMeshData>();
				std::string previewValue = "None";
				if (component.NavMeshDataHandle != Constants::InvalidUUID)
				{
					auto navMeshAsset = Application::Instance().GetAssetManager().GetAsset<NavigationMeshData>(component.NavMeshDataHandle);
					if (navMeshAsset)
						previewValue = navMeshAsset->GetName();
				}

				if (UI::PropertyGrid::BeginComboBox("Navigation Mesh", previewValue))
				{
					if (UI::PropertyGrid::ComboBoxItem("None", component.NavMeshDataHandle == Constants::InvalidUUID))
						component.NavMeshDataHandle = Constants::InvalidUUID;
					for (auto& navMeshAsset : navMeshes)
					{
						if (UI::PropertyGrid::ComboBoxItem(navMeshAsset->GetName(), component.NavMeshDataHandle == navMeshAsset->GetUUID()))
							component.NavMeshDataHandle = navMeshAsset->GetUUID();
					}
					UI::PropertyGrid::EndComboBox();
				}
				
				if (UI::PropertyGrid::BeginComboBox("Path Mode", (component.Type == NavigationMeshModifierComponent::ModifierType::Walkable) ? "Walkable" : "Not Walkable"))
				{
					if (UI::PropertyGrid::ComboBoxItem("Walkable", component.Type == NavigationMeshModifierComponent::ModifierType::Walkable))
						component.Type = NavigationMeshModifierComponent::ModifierType::Walkable;
					if (UI::PropertyGrid::ComboBoxItem("Not Walkable", component.Type == NavigationMeshModifierComponent::ModifierType::NotWalkable))
						component.Type = NavigationMeshModifierComponent::ModifierType::NotWalkable;

					UI::PropertyGrid::EndComboBox();
				}

				UI::PropertyGrid::Checkbox("Apply To Children", component.ApplyToChildren);

				UI::PropertyGrid::End();
			}
		}
	};

}