#include "efpch.h"
#include "ProjectSettingsDialog.h"

#include <imgui/imgui.h>

namespace Ember {

	ProjectSettingsDialog::ProjectSettingsDialog()
	{
		// TODO: Move items to property grid
		// TODO: Save all settings the moment they are changed (hook up settings to actual values)
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

			// --- LEFT PANE (Categories) ---
			ImGui::BeginChild("CategoryPane", ImVec2(150, splitHeight), true);
			RenderLeftPane();
			ImGui::EndChild();

			ImGui::SameLine();

			// --- RIGHT PANE (Settings Content) ---
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
		// Draw the corresponding content based on the selected tab
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

		// Just some dummy data for now to show the layout
		static char projectName[128] = "Ember Forge Project";
		ImGui::InputText("Project Name", projectName, IM_ARRAYSIZE(projectName));
	}

	void ProjectSettingsDialog::RenderPhysicsSettings()
	{
		ImGui::Text("Physics 3D");
		ImGui::Separator();
		ImGui::Spacing();

		// NOTE: In your real engine, you would fetch these references directly from your PhysicsSystem.
		// I am using static variables here just so the UI functions immediately when you paste it in.
		static float gravity[3] = { 0.0f, -9.81f, 0.0f };
		static int posIters = 8;
		static int velIters = 15;

		ImGui::TextDisabled("Simulation");
		ImGui::DragFloat3("Default Gravity", gravity, 0.1f);

		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::TextDisabled("Solver");
		ImGui::DragInt("Position Iterations", &posIters, 1, 1, 32);
		ImGui::DragInt("Velocity Iterations", &velIters, 1, 1, 32);
	}

}