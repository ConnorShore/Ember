#pragma once


#include "ComponentUI.h"
#include "Ui/PropertyGrid.h"

#include <Ember/Event/UIEvent.h>
#include <Ember/Animation/AnimationStateMachine.h>

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
			if (UI::PropertyGrid::Begin("AnimatorComponentProps"))
			{
				RenderStateMachinePicker(component);
				RenderStatePicker(component);

				float currentTime = component.CurrentTime.Seconds();
				if (UI::PropertyGrid::Float("Current Time", currentTime, 0.05f, 0.0f, 10000.0f))
					component.CurrentTime = currentTime;

				UI::PropertyGrid::End();
			}
		}

		inline void ResetRuntimeState(AnimatorComponent& component)
		{
			component.CurrentTime = 0.0f;
			component.PreviousTime = 0.0f;
			component.PreviousStateName.clear();
			component.CurrentBlendTime = 0.0f;
			component.ActiveBlendDuration = 0.0f;
			component.IsBlending = false;
		}

		inline void RenderStateMachinePicker(AnimatorComponent& component)
		{
			SharedPtr<AnimationStateMachine> selectedStateMachine = nullptr;
			std::string stateMachineName;
			if (component.AnimationStateMachineHandle == Constants::InvalidUUID)
				stateMachineName = "None";
			else
			{
				selectedStateMachine = m_AssetManager.GetAsset<AnimationStateMachine>(component.AnimationStateMachineHandle);
				stateMachineName = selectedStateMachine ? selectedStateMachine->GetName() : "Missing";
			}

			if (UI::PropertyGrid::BeginComboBox("State Machine", stateMachineName.c_str()))
			{
				if (UI::PropertyGrid::ComboBoxItem("None", component.AnimationStateMachineHandle == Constants::InvalidUUID))
				{
					component.AnimationStateMachineHandle = Constants::InvalidUUID;
					component.CurrentStateName.clear();
					ResetRuntimeState(component);
				}

				ImGui::Separator();

				auto stateMachines = m_AssetManager.GetAssetsOfType<AnimationStateMachine>();
				for (auto& stateMachine : stateMachines)
				{
					bool isSelected = component.AnimationStateMachineHandle == stateMachine->GetUUID();
					if (UI::PropertyGrid::ComboBoxItem(stateMachine->GetName().c_str(), isSelected))
					{
						component.AnimationStateMachineHandle = stateMachine->GetUUID();
						component.CurrentStateName = stateMachine->GetDefaultState();
						ResetRuntimeState(component);
					}
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				UI::PropertyGrid::EndComboBox();
			}

			if (selectedStateMachine)
			{
				ImGui::SameLine();
				if (ImGui::Button("Edit"))
				{
					// Open the ASM editor for this state machine
					m_Context->RequestAnimationStateOpenPath = selectedStateMachine->GetFilePath();
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
			auto stateMachine = component.AnimationStateMachineHandle != Constants::InvalidUUID ? m_AssetManager.GetAsset<AnimationStateMachine>(component.AnimationStateMachineHandle) : nullptr;
			if (!stateMachine)
				return;

			std::string stateName = component.CurrentStateName.empty() ? "Default" : component.CurrentStateName;
			if (UI::PropertyGrid::BeginComboBox("Current State", stateName.c_str()))
			{
				if (UI::PropertyGrid::ComboBoxItem("Default", component.CurrentStateName.empty()))
				{
					component.CurrentStateName.clear();
					ResetRuntimeState(component);
				}

				ImGui::Separator();

				for (const auto& [name, state] : stateMachine->GetStates())
				{
					bool isSelected = component.CurrentStateName == name;
					if (UI::PropertyGrid::ComboBoxItem(name.c_str(), isSelected))
					{
						component.CurrentStateName = name;
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