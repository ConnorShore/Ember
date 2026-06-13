#pragma once


#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"

#include <Ember/Event/UIEvent.h>
#include <Ember/Animation/AnimationController.h>

#include <imgui/imgui.h>

namespace Ember {

	class AnimatorComponentUI : public ComponentUI<AnimatorComponent>
	{
	public:
		AnimatorComponentUI(EditorContext* context) : ComponentUI(context) { m_CanRemove = false; }
		inline const char* GetName() const override { return "Animator Component"; }

	protected:
		inline void RenderComponentImpl(AnimatorComponent& component) override
		{
			if (component.LayerStates.empty())
				component.LayerStates.emplace_back();

			if (UI::PropertyGrid::Begin("AnimatorComponentProps"))
			{
				RenderStateMachinePicker(component);
				RenderStatePicker(component);

				float currentTime = component.LayerStates[0].CurrentTime.Seconds();
				if (UI::PropertyGrid::Float("Current Time", currentTime, 0.05f, 0.0f, 10000.0f))
					component.LayerStates[0].CurrentTime = currentTime;

				UI::PropertyGrid::End();
			}
		}

		inline void ResetRuntimeState(AnimatorComponent& component)
		{
			if (component.LayerStates.empty())
				component.LayerStates.emplace_back();

			component.LayerStates[0].CurrentTime = 0.0f;
			component.LayerStates[0].PreviousTime = 0.0f;
			component.LayerStates[0].PreviousStateId = Constants::InvalidUUID;
			component.LayerStates[0].CurrentBlendTime = 0.0f;
			component.LayerStates[0].ActiveBlendDuration = 0.0f;
			component.LayerStates[0].IsBlending = false;
		}

		inline void RenderStateMachinePicker(AnimatorComponent& component)
		{
			SharedPtr<AnimationController> selectedController = nullptr;
			std::string stateMachineName;
			if (component.ControllerHandle == Constants::InvalidUUID)
				stateMachineName = "None";
			else
			{
				selectedController = m_AssetManager.GetAsset<AnimationController>(component.ControllerHandle);
				stateMachineName = selectedController ? selectedController->GetName() : "Missing";
			}

			if (UI::PropertyGrid::BeginComboBox("Controller", stateMachineName.c_str()))
			{
				if (UI::PropertyGrid::ComboBoxItem("None", component.ControllerHandle == Constants::InvalidUUID))
				{
					component.ControllerHandle = Constants::InvalidUUID;
					if (component.LayerStates.empty())
						component.LayerStates.emplace_back();
					component.LayerStates[0].CurrentStateId = Constants::InvalidUUID;
					ResetRuntimeState(component);
				}

				ImGui::Separator();

				auto controllers = m_AssetManager.GetAssetsOfType<AnimationController>();
				for (auto& controller : controllers)
				{
					bool isSelected = component.ControllerHandle == controller->GetUUID();
					if (UI::PropertyGrid::ComboBoxItem(controller->GetName().c_str(), isSelected))
					{
						component.ControllerHandle = controller->GetUUID();
						if (component.LayerStates.empty())
							component.LayerStates.emplace_back();
						if (!controller->GetLayers().empty())
							component.LayerStates[0].CurrentStateId = controller->GetLayers()[0].StateMachine.GetDefaultState();
						else
							component.LayerStates[0].CurrentStateId = Constants::InvalidUUID;
						ResetRuntimeState(component);
					}
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				UI::PropertyGrid::EndComboBox();
			}

			if (selectedController)
			{
				ImGui::SameLine();
				if (ImGui::Button("Edit"))
				{
					// Open the ASM editor for this state machine
					m_Context->RequestAnimationStateOpenPath = selectedController->GetFilePath();
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("New"))
			{
				// TODO: Create a new ASM asset and open it for editing
			}
		}

		inline void RenderStatePicker(AnimatorComponent& component)
		{
			if (component.LayerStates.empty())
				component.LayerStates.emplace_back();

			auto controller = component.ControllerHandle != Constants::InvalidUUID ? m_AssetManager.GetAsset<AnimationController>(component.ControllerHandle) : nullptr;
			if (!controller || controller->GetLayers().empty())
				return;

			auto& runtime = component.LayerStates[0];
			auto& stateMachine = controller->GetLayers()[0].StateMachine;

			// Find display name for current state
			std::string currentDisplayName = "Default";
			if (runtime.CurrentStateId != Constants::InvalidUUID)
			{
				const auto& states = stateMachine.GetStates();
				auto it = states.find(runtime.CurrentStateId);
				if (it != states.end())
					currentDisplayName = it->second.Name;
			}

			if (UI::PropertyGrid::BeginComboBox("Current State", currentDisplayName.c_str()))
			{
				if (UI::PropertyGrid::ComboBoxItem("Default", runtime.CurrentStateId == Constants::InvalidUUID))
				{
					runtime.CurrentStateId = Constants::InvalidUUID;
					ResetRuntimeState(component);
				}

				ImGui::Separator();

				for (const auto& [stateId, state] : stateMachine.GetStates())
				{
					bool isSelected = runtime.CurrentStateId == stateId;
					if (UI::PropertyGrid::ComboBoxItem(state.Name.c_str(), isSelected))
					{
						runtime.CurrentStateId = stateId;
						ResetRuntimeState(component);
					}
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				UI::PropertyGrid::EndComboBox();
			}
		}
	};

}