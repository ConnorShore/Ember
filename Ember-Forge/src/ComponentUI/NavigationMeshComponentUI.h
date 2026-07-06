#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"

#include <Ember/AI/NavigationMeshBuilder.h>
#include <Ember/Asset/Serializers/NavigationMeshSerializer.h>
#include <Ember/Core/ProjectManager.h>

#include <imgui/imgui.h>

#include <filesystem>

namespace Ember {

	class NavigationMeshComponentUI : public ComponentUI<NavigationMeshComponent>
	{
	public:
		NavigationMeshComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Navigation Mesh Component"; }

	protected:
		inline void RenderComponentImpl(NavigationMeshComponent& component) override
		{
			ImGui::TextDisabled("Bound Settings");
			ImGui::Spacing();
			if (UI::PropertyGrid::Begin("NavigationBoundMeshProps"))
			{
				UI::PropertyGrid::Float3("Bounds Size", component.BoundsSize, 0.1f, 0.1f);
				UI::PropertyGrid::Float3("Bounds Center Offset", component.BoundsCenterOffset, 0.1f, 0.1f);
				UI::PropertyGrid::End();
			}

			ImGui::TextDisabled("Cell Settings");
			ImGui::Spacing();
			if (UI::PropertyGrid::Begin("NavigationCellMeshProps"))
			{
				UI::PropertyGrid::Float("Size", component.BakeSettings.CellSize, 0.1f, 0.1f);
				UI::PropertyGrid::Float("Height", component.BakeSettings.CellHeight, 0.1f, 0.1f);
				UI::PropertyGrid::End();
			}

			ImGui::TextDisabled("Agent Settings");
			ImGui::Spacing();
			if (UI::PropertyGrid::Begin("NavigationAgentMeshProps"))
			{
				UI::PropertyGrid::Float("Height", component.BakeSettings.AgentHeight, 0.1f, 0.01f);
				UI::PropertyGrid::Float("Radius", component.BakeSettings.AgentRadius, 0.1f, 0.01f);
				UI::PropertyGrid::Float("Max Climb", component.BakeSettings.AgentMaxClimb, 0.1f, 0.01f);
				UI::PropertyGrid::Float("Max Slope", component.BakeSettings.AgentMaxSlope, 0.1f, 0.0f, 90.0f);
				UI::PropertyGrid::End();
			}

			// TODO: Disable if bake settings are equal to the last bake settings (to avoid unnecessary bakes)
			if (ImGui::Button("Bake"))
			{
				SharedPtr<NavigationMeshData> bakedMesh = nullptr;
				if (component.NavMeshDataHandle == Constants::InvalidUUID)
				{
					// Create a new nav mesh asset
					std::string sceneName = m_Context->ActiveScene()->GetName();
					std::string entityName = m_Context->SelectedEntity.GetName();
					std::string newNavMeshName = std::format("NavMesh_{}_{}", sceneName, entityName);
					std::string newNavMeshFileName = newNavMeshName + ".ebnav";
					auto newNavFilePath = ProjectManager::GetActive()->GetDefaultDirectoryForAsset(AssetType::NavMeshData) / newNavMeshFileName;
					std::filesystem::create_directories(newNavFilePath.parent_path());
					auto newNavMeshAsset = m_AssetManager.Create<NavigationMeshData>(newNavMeshName, newNavFilePath.string());
					newNavMeshAsset->SetIsEngineAsset(false);
					component.NavMeshDataHandle = newNavMeshAsset->GetUUID();
				}

				// Load the existing nav mesh asset
				bakedMesh = m_AssetManager.GetAsset<NavigationMeshData>(component.NavMeshDataHandle);
				if (!bakedMesh)
				{
					auto evt = UINotificationEvent("Failed to load navigation mesh asset.", UINotificationEvent::Severity::Warning);
					m_Context->EventCallback(evt);
					return;
				}

				// Set the bake settings on the baked mesh asset
				bakedMesh->SetBakeSettings(component.BakeSettings);

				// Bake the nav mesh using the current scene geometry and the specified bake settings.
				auto buildResult = NavigationMeshBuilder::BuildNavigationMesh(m_Context->ActiveScene(), m_Context->SelectedEntity, component.BakeSettings);
				if (!buildResult.Success)
				{
					auto evt = UINotificationEvent(buildResult.Error.empty() ? "Navigation mesh bake failed." : buildResult.Error, UINotificationEvent::Severity::Warning);
					m_Context->EventCallback(evt);
					return;
				}

				// Store baked Detour bytes directly on the asset.
				bakedMesh->SetRawDataBlob(std::move(buildResult.RawDataBlob));

				// Initialize nav mesh from the data
				bakedMesh->InitializeFromRawData();

				// Serialize source metadata + sidecar blob.
				if (!NavigationMeshSerializer::SerializeSource(bakedMesh->GetFilePath(), bakedMesh))
				{
					auto evt = UINotificationEvent("Failed to serialize the baked navigation mesh.", UINotificationEvent::Severity::Warning);
					m_Context->EventCallback(evt);
					return;
				}
			}

			if (component.NavMeshDataHandle != Constants::InvalidUUID)
			{
				if (ImGui::Button("Clear"))
				{
					// Clear the handle from the component
					component.NavMeshDataHandle = Constants::InvalidUUID;

					// TODO: Remove the asset from the asset manager or just clear the handle and data
				}
			}
		}
	};

}