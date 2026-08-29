#include "efpch.h"
#include "ProjectSettingsDialog.h"

#include "UI/PropertyGrid.h"
#include "UI/DragDropTypes.h"

#include <Ember/Core/ProjectManager.h>
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

		if (ImGui::Selectable("Input", m_SelectedCategory == Category::Input))
			m_SelectedCategory = Category::Input;

		if (ImGui::Selectable("Physics", m_SelectedCategory == Category::Physics))
			m_SelectedCategory = Category::Physics;

		if (ImGui::Selectable("Rendering", m_SelectedCategory == Category::Rendering))
			m_SelectedCategory = Category::Rendering;
	}

	void ProjectSettingsDialog::RenderRightPane()
	{
		switch (m_SelectedCategory)
		{
		case Category::General: RenderGeneralSettings(); break;
		case Category::Input: RenderInputSettings(); break;
		case Category::Physics: RenderPhysicsSettings(); break;
		case Category::Rendering: RenderRenderingSettings(); break;
		}
	}

	void ProjectSettingsDialog::RenderGeneralSettings()
	{
		ImGui::Text("General Settings");
		ImGui::Separator();
		ImGui::Spacing();

		std::string projName = ProjectManager::GetActive()->GetConfig().ProjectName;
		static char projectName[128] = "";

		// Set projectName to projName's value
		for (size_t i = 0; i < IM_ARRAYSIZE(projectName); i++)
			projectName[i] = i < projName.size() ? projName[i] : '\0';

		if (ImGui::InputText("Project Name", projectName, IM_ARRAYSIZE(projectName)))
		{
			// TODO: Handle project renaming (do we need to delete old project file and save a new one? Or just update the name in the config and save on next project save?)
			ProjectManager::GetActive()->GetConfig().ProjectName = std::string(projectName);
		}

		// List order of scenes (like waypoints UI style) (so can call SceneManager.LoadNextScene() in lua)
		ImGui::Spacing();
		ImGui::Text("Scenes In Build");
		ImGui::Separator();

		if (UI::PropertyGrid::Begin("BuildScenesGrid"))
		{
			// 1. Tell the widget how to get a name
			auto nameResolver = [](UUID uuid) -> std::string {
				auto asset = Application::Instance().GetAssetManager().GetAssetBase(uuid);
				return asset ? asset->GetName() : "Invalid Scene";
				};

			// 2. Gather all available scenes so the dropdown has items to display!
			// (Assuming you have a GetAssetsOfType method or similar)
			auto scenes = Application::Instance().GetAssetManager().GetAssetsOfType<Scene>();
			std::vector<UUID> availableScenes;
			for (auto& scene : scenes)
			{
				availableScenes.push_back(scene->GetUUID());
			}

			// 3. Get the project config list to modify
			auto project = ProjectManager::GetActive();
			auto& buildScenes = project->GetScenesInBuild();

			// 4. Draw the ComboBox array
			UI::PropertyGrid::DynamicUUIDArrayComboBox("Scenes", "Index", buildScenes, availableScenes, nameResolver);

			// Set first build scene as the default scene in the project (TODO)
			if (!buildScenes.empty())
			{
				project->SetStartScene(buildScenes[0]);
			}

			UI::PropertyGrid::End();
		}

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

		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::TextDisabled("Collision Categories");

		// Use a child window with a border so it looks clean inside the right pane.
		// ImVec2(0, 0) tells ImGui to fill the remaining width and height perfectly.
		ImGui::BeginChild("CollisionCategoriesSection", ImVec2(0, 0), true);

		auto& filterManager = ProjectManager::GetActive()->GetCollisionFilterManager();

		// ReactPhysics3D uses a 16-bit integer for masks, so we iterate exactly 16 times.
		for (uint32_t i = 0; i < 16; i++)
		{
			ImGui::PushID(i);

			// Fixed width for the label so the input boxes align perfectly into a neat column
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Layer %2d", i+1);
			ImGui::SameLine(80.0f);

			// Render an input box for the filter name, pre-filled with the current name from the filter manager
			std::string currentName = filterManager.GetFilterNameBySlot(i);
			char buffer[64];
			strncpy_s(buffer, sizeof(buffer), currentName.c_str(), _TRUNCATE);

			// Dynamically size the input box to stretch, but leave 60px for the Clear button
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);

			// Update the slot name instantly as the user types
			if (ImGui::InputText("##LayerName", buffer, sizeof(buffer)))
			{
				filterManager.SetFilterNameAtSlot(i, std::string(buffer));
			}

			// The Clear button quickly wipes the slot back to an empty string
			ImGui::SameLine();
			if (ImGui::Button("Clear"))
			{
				filterManager.SetFilterNameAtSlot(i, "");
			}

			ImGui::PopID();
		}

		ImGui::EndChild();

	}

	void ProjectSettingsDialog::RenderRenderingSettings()
	{
		ImGui::Text("Rendering");
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextDisabled("Render Layers");

		// Use a child window with a border so it looks clean inside the right pane.
		// ImVec2(0, 0) tells ImGui to fill the remaining width and height perfectly.
		ImGui::BeginChild("RenderLayersSection", ImVec2(0, 0), true);

		auto& filterManager = ProjectManager::GetActive()->GetRenderFilterManager();

		// ReactPhysics3D uses a 16-bit integer for masks, so we iterate exactly 16 times.
		for (uint32_t i = 0; i < 16; i++)
		{
			ImGui::PushID(i);

			// Fixed width for the label so the input boxes align perfectly into a neat column
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Layer %2d", i+1);
			ImGui::SameLine(80.0f);

			// Render textbox for the layer name, with a Clear button next to it
			std::string currentName = filterManager.GetFilterNameBySlot(i);
			char buffer[64];
			strncpy_s(buffer, sizeof(buffer), currentName.c_str(), _TRUNCATE);

			// Dynamically size the input box to stretch, but leave 60px for the Clear button
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);

			// Update the slot name instantly as the user types
			if (ImGui::InputText("##LayerName", buffer, sizeof(buffer)))
			{
				filterManager.SetFilterNameAtSlot(i, std::string(buffer));
			}

			// The Clear button quickly wipes the slot back to an empty string
			ImGui::SameLine();
			if (ImGui::Button("Clear"))
			{
				filterManager.SetFilterNameAtSlot(i, "");
			}

			ImGui::PopID();
		}

		ImGui::EndChild();
	}

	void ProjectSettingsDialog::RenderInputSettings()
	{
		ImGui::Text("Input");
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextDisabled("Input Actions");

		ImGui::BeginChild("InputSection", ImVec2(0, 0), true);
		
		// TODO: Need to implement a system for managing input actions, including adding, removing, and editing actions.
		//  1. Need to be able to add/remove action
		//  2. Action should have column for name
		//  3. Action should have column for type (Button, Axis, etc.)
		//  4. Action should have column for the binding (based on the type)
		//   a. Need way to "Press Key to Set Binding" type of input box

		ImGui::EndChild();
	}

}