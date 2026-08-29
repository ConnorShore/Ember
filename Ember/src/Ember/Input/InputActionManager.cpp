#include "ebpch.h"
#include "InputActionManager.h"
#include "Input.h"

namespace Ember {

	namespace {

		bool IsTriggerActive(const InputTrigger& trigger)
		{
			switch (trigger.Device)
			{
			case InputDevice::Keyboard:
				if (std::holds_alternative<KeyCode>(trigger.ControlId))
				{
					KeyCode key = std::get<KeyCode>(trigger.ControlId);
					return Input::IsKeyDown(key) && Input::GetActiveModifiers() == trigger.RequiredModifier;
				}
				break;
			case InputDevice::Mouse:
				if (std::holds_alternative<MouseControl>(trigger.ControlId))
				{
					MouseControl control = std::get<MouseControl>(trigger.ControlId);
					return Input::IsMouseControlDown(control) && Input::GetActiveModifiers() == trigger.RequiredModifier;
				}
				break;
			default:
				EB_CORE_ERROR("InputActionManager::Evaluate: Unknown input device type: {0}", static_cast<int>(trigger.Device));
				break;
			}
			return false;
		}

	}

	void InputActionManager::Evaluate()
	{
		for (int i = 0; i < m_Actions.size(); ++i)
		{
			InputActionState& state = m_ActionStates[i];
			const InputAction& action = m_Actions[i];
			bool wasDown = state.IsDown;
			state.IsDown = false;
			state.JustPressed = false;
			state.JustReleased = false;

			for (const InputTrigger& trigger : action.Triggers)
			{
				if (IsTriggerActive(trigger))
				{
					state.IsDown = true;
					state.LastDevice = trigger.Device;
					if (!wasDown)
						state.JustPressed = true;
					break;
				}
			}

			if (wasDown && !state.IsDown)
				state.JustReleased = true;
		}
	}

	bool InputActionManager::IsActionDown(std::string_view actionName)
	{
		return m_ActionStates[GetActionIndex(actionName)].IsDown;
	}

	bool InputActionManager::IsActionPressed(std::string_view actionName)
	{
		return m_ActionStates[GetActionIndex(actionName)].JustPressed;
	}

	bool InputActionManager::IsActionReleased(std::string_view actionName)
	{
		return m_ActionStates[GetActionIndex(actionName)].JustReleased;
	}

	int InputActionManager::GetActionIndex(std::string_view actionName)
	{
		for (int i = 0; i < m_Actions.size(); ++i)
		{
			if (m_Actions[i].Name == actionName)
				return i;
		}

		EB_CORE_ERROR("InputActionManager::GetActionIndex: Action not found: {0}", actionName);
		return -1;
	}

	void InputActionManager::AddAction(const InputAction& action)
	{
		m_Actions.push_back(action);
		m_ActionStates.push_back(InputActionState());
	}

	void InputActionManager::EditActionName(int index, std::string_view newName)
	{
		EB_CORE_ASSERT(index >= 0 && index < m_Actions.size(), "InputActionManager::EditActionName: Index out of range");
		m_Actions[index].Name = newName;
	}

	void InputActionManager::RemoveAction(int index)
	{
		EB_CORE_ASSERT(index >= 0 && index < m_Actions.size(), "InputActionManager::RemoveAction: Index out of range");
		m_Actions.erase(m_Actions.begin() + index);
		m_ActionStates.erase(m_ActionStates.begin() + index);
	}

	void InputActionManager::RemoveAction(std::string_view actionName)
	{
		RemoveAction(GetActionIndex(actionName));
	}

	void InputActionManager::AddTrigger(int actionIndex, const InputTrigger& trigger)
	{
		m_Actions[actionIndex].Triggers.push_back(trigger);
		m_ActionStates[actionIndex] = InputActionState(); // Reset state for the action
	}

	void InputActionManager::AddTrigger(std::string_view actionName, const InputTrigger& trigger)
	{
		AddTrigger(GetActionIndex(actionName), trigger);
	}

	void InputActionManager::UpdateTrigger(int actionIndex, int triggerIndex, const InputTrigger& trigger)
	{
		EB_CORE_ASSERT(actionIndex >= 0 && actionIndex < m_Actions.size(), "InputActionManager::UpdateTrigger: Action index out of range");
		EB_CORE_ASSERT(triggerIndex >= 0 && triggerIndex < m_Actions[actionIndex].Triggers.size(), "InputActionManager::UpdateTrigger: Trigger index out of range");

		m_Actions[actionIndex].Triggers[triggerIndex] = trigger;
		m_ActionStates[actionIndex] = InputActionState(); // Reset state for the action
	}

	void InputActionManager::RemoveTrigger(int actionIndex, int triggerIndex)
	{
		EB_CORE_ASSERT(actionIndex >= 0 && actionIndex < m_Actions.size(), "InputActionManager::RemoveTrigger: Action index out of range");
		m_Actions[actionIndex].Triggers.erase(m_Actions[actionIndex].Triggers.begin() + triggerIndex);
		m_ActionStates[actionIndex] = InputActionState(); // Reset state for the action
	}

	void InputActionManager::RemoveTrigger(std::string_view actionName, int triggerIndex)
	{
		RemoveTrigger(GetActionIndex(actionName), triggerIndex);
	}

	void InputActionManager::ClearActions()
	{
		m_Actions.clear();
		m_ActionStates.clear();
	}

}