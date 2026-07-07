#pragma once

#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"
#include "Utils/ActiveNavMeshRenderer.h"

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
			if (UI::PropertyGrid::Begin("DrawNavigationMeshProps"))
			{
				bool drawNavMesh = ActiveNavMeshRenderer::GetEnabled();
				if (UI::PropertyGrid::Checkbox("Draw Navigation Mesh", drawNavMesh))
					ActiveNavMeshRenderer::SetEnabled(drawNavMesh);

				UI::PropertyGrid::End();
			}

			ImGui::TextDisabled("Bound Settings");
			ImGui::Spacing();
			if (UI::PropertyGrid::Begin("NavigationBoundMeshProps"))
			{
				UI::PropertyGrid::Float3("Bounds Size", component.BoundsSize, 0.1f, 0.1f);
				UI::PropertyGrid::Float3("Bounds Center Offset", component.BoundsCenterOffset, 0.1f, 0.1f);
				UI::PropertyGrid::End();
			}

			ImGui::TextDisabled("Region Settings");
			ImGui::Spacing();
			if (UI::PropertyGrid::Begin("NavigationCellMeshProps"))
			{
				UI::PropertyGrid::Float("Cell Size", component.BakeSettings.CellSize, 0.1f, 0.1f);
				UI::PropertyGrid::Float("Cell Height", component.BakeSettings.CellHeight, 0.1f, 0.1f);
				UI::PropertyGrid::Float("Region Min Size", component.BakeSettings.RegionMinSize, 0.1f, 0.1f);
				UI::PropertyGrid::Float("Edge Max Error", component.BakeSettings.EdgeMaxError, 0.1f, 0.01f);
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
			SharedPtr<NavigationMeshData> navMeshAsset = nullptr;
			if (component.NavMeshDataHandle != Constants::InvalidUUID)
				navMeshAsset = m_AssetManager.GetAsset<NavigationMeshData>(component.NavMeshDataHandle);

			if (UI::PropertyGrid::Begin("NavMeshData"))
			{
				if (navMeshAsset)
					UI::PropertyGrid::DisabledInputText("Navigation Data", navMeshAsset->GetName());
				else
					UI::PropertyGrid::DisabledInputText("Navigation Data", "None");

				UI::PropertyGrid::End();
			}

			// Make bake and clear buttons both 1/2 the width of the panel after accounting for label "Navigation Data"
			float labelWidth = ImGui::CalcTextSize("Navigation Data").x + ImGui::GetStyle().FramePadding.x * 2.0f;
			float buttonWidth = (ImGui::GetContentRegionAvail().x - labelWidth - ImGui::GetStyle().ItemSpacing.x) / 2.0f;

			// Shift button starting position to the right so it aligns with the input text
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + labelWidth);

			if (navMeshAsset)
			{
				if (ImGui::Button("Bake", ImVec2(buttonWidth, 0)))
					BakeNavigationMesh(component);

				ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x);
				if (ImGui::Button("Clear", ImVec2(buttonWidth, 0)))
				{
					// Delete the nav mesh asset from the project
					m_AssetManager.RemoveAsset(component.NavMeshDataHandle);
					component.NavMeshDataHandle = Constants::InvalidUUID;
				}
			}
			else
			{
				if (ImGui::Button("Bake", ImVec2(buttonWidth * 2, 0)))
					BakeNavigationMesh(component);
			}
		}

	private:
		void BakeNavigationMesh(NavigationMeshComponent& component)
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
			}

			// Load mesh as non-engine asset
			m_AssetManager.Load<NavigationMeshData>(bakedMesh->GetFilePath(), false);
		}
	};

}