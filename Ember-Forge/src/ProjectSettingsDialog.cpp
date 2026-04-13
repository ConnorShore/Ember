#include "efpch.h"
#include "ProjectSettingsDialog.h"

#include "UI/PropertyGrid.h"

#include <Ember/ECS/System/PhysicsSystem.h>
#include <imgui/imgui.h>

namespace Ember {

	ProjectSettingsDialog::ProjectSettingsDialog()
	{
	}

	ProjectSettingsDialog::~ProjectSettingsDialog()
	{
	}

	void ProjectSettingsDialog::OnImGuiRender()
	{
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

		// Set a fixed window size so the split layout has room to breathe
		ImGui::SetNextWindowSize(ImVec2(700, 450), ImGuiCond_Appearing);

		bool isOpen = true;
		if (ImGui::BeginPopupModal(m_PopupName.c_str(), &isOpen, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize))
		{
			if (!isOpen)
			{
				ImGui::CloseCurrentPopup();
				return;
			}

			ImVec2 contentRegion = ImGui::GetContentRegionAvail();
			float splitHeight = contentRegion.y;

			// Left pane (Category List)
			ImGui::BeginChild("CategoryPane", ImVec2(150, splitHeight), true);
			RenderLeftPane();
			ImGui::EndChild();

			ImGui::SameLine();

			// Right pane (Content)
			ImGui::BeginChild("ContentPane", ImVec2(0, splitHeight), true);
			RenderRightPane();
			ImGui::EndChild();

			ImGui::EndPopup();
		}
	}

	void ProjectSettingsDialog::RenderLeftPane()
	{
		if (ImGui::Selectable("General", m_SelectedCategory == Category::General))
			m_SelectedCategory = Category::General;

		if (ImGui::Selectable("Physics", m_SelectedCategory == Category::Physics))
			m_SelectedCategory = Category::Physics;
	}

	void ProjectSettingsDialog::RenderRightPane()
	{
		switch (m_SelectedCategory)
		{
		case Category::General: RenderGeneralSettings(); break;
		case Category::Physics: RenderPhysicsSettings(); break;
		}
	}

	void ProjectSettingsDialog::RenderGeneralSettings()
	{
		ImGui::Text("General Settings");
		ImGui::Separator();
		ImGui::Spacing();

		// TODO: Hook up project name to actual project data
		static char projectName[128] = "Ember Forge Project";
		ImGui::InputText("Project Name", projectName, IM_ARRAYSIZE(projectName));
	}

	void ProjectSettingsDialog::RenderPhysicsSettings()
	{
		ImGui::Text("Physics 3D");
		ImGui::Separator();
		ImGui::Spacing();

		auto& physicsSettings = Application::Instance().GetSystem<PhysicsSystem>()->GetSettings();

		ImGui::TextDisabled("Simulation");
		if (UI::PropertyGrid::Begin("Physics Simulation Settings"))
		{
			UI::PropertyGrid::Float("Default Gravity Strength", physicsSettings.GravityStrength, 0.01f, 0.0f, 1000.0f);
			UI::PropertyGrid::Float3("Default Gravity Vector", physicsSettings.GravityVector, 0.01f, -1.0f, 1.0f);

			UI::PropertyGrid::End();
		}

		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::TextDisabled("Solver");
		if (UI::PropertyGrid::Begin("Physics Solver Settings"))
		{
			UI::PropertyGrid::UInt("Position Iterations", physicsSettings.PositionSolverIterations, 1, 1, 32);
			UI::PropertyGrid::UInt("Velocity Iterations", physicsSettings.VelocitySolverIterations, 1, 1, 32);

			UI::PropertyGrid::End();
		}
	}

}