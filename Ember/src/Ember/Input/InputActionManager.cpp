#include "ebpch.h"
#include "InputActionManager.h"
#include "Input.h"

namespace Ember {

	namespace {

		// A half-axis trigger reports 0..1 within its own half and nothing at all outside it, so a pair
		// of actions can drive one axis in opposite directions.
		float DirectionalStrength(float value, AxisDirection direction)
		{
			switch (direction)
			{
			case AxisDirection::Positive: return Math::Max(value, 0.0f);
			case AxisDirection::Negative: return Math::Max(-value, 0.0f);
			default:                      return value;
			}
		}

		// How hard a control has to be pushed before a digital read counts it as pressed. Anything
		// that is already on/off reports 1.0, so only the analog gamepad axes really need this.
		float GetTriggerActuation(const InputTrigger& trigger)
		{
			if (trigger.Device != InputDevice::Gamepad || !std::holds_alternative<GamepadAxis>(trigger.ControlId))
				return 1.0f;

			GamepadAxis axis = std::get<GamepadAxis>(trigger.ControlId);
			return Input::IsTriggerAxis(axis)
				? Input::ActuationThreshold(Input::TriggerForAxis(axis))
				: Input::ActuationThreshold(Input::StickForAxis(axis));
		}

		// Zero when the trigger's control is not engaged; signed only for a Full-direction axis.
		float GetTriggerStrength(const InputTrigger& trigger)
		{
			switch (trigger.Device)
			{
			case InputDevice::Keyboard:
				if (std::holds_alternative<KeyCode>(trigger.ControlId))
				{
					KeyCode key = std::get<KeyCode>(trigger.ControlId);
					bool active = Input::IsKeyDown(key) || Input::IsKeyPressed(key);
					active = active && ((Input::GetActiveModifiers() & trigger.RequiredModifiers) == trigger.RequiredModifiers);
					return active ? 1.0f : 0.0f;
				}
				break;
			case InputDevice::Mouse:
				if (std::holds_alternative<MouseControl>(trigger.ControlId))
				{
					MouseControl control = std::get<MouseControl>(trigger.ControlId);
					bool active = Input::IsMouseControlDown(control) || Input::IsMouseControlPressed(control);
					active = active && ((Input::GetActiveModifiers() & trigger.RequiredModifiers) == trigger.RequiredModifiers);
					return active ? 1.0f : 0.0f;
				}
				break;
			case InputDevice::Gamepad:
				if (std::holds_alternative<GamepadButton>(trigger.ControlId))
				{
					GamepadButton button = std::get<GamepadButton>(trigger.ControlId);
					const GamepadButtonMask mask = static_cast<GamepadButtonMask>(1 << static_cast<GamepadButtonType>(button));
					for (size_t i = 0; i < Input::MaxGamepads; ++i)
					{
						const GamepadState& state = Input::GetGamepadState(i);
						if ((state.Down & mask) != 0)
							return 1.0f;
					}
				}
				else if (std::holds_alternative<GamepadAxis>(trigger.ControlId))
				{
					GamepadAxis axis = std::get<GamepadAxis>(trigger.ControlId);
					for (size_t i = 0; i < Input::MaxGamepads; ++i)
					{
						// Input::ProcessGamepads has already conditioned the axis, so anything
						// non-zero here is real movement.
						const GamepadState& state = Input::GetGamepadState(i);
						float strength = DirectionalStrength(state.Axis[static_cast<size_t>(axis)], trigger.Direction);
						if (strength != 0.0f)
							return strength;
					}
				}
				break;
			default:
				EB_CORE_ERROR("InputActionManager::GetTriggerStrength: Unknown input device type: {0}", static_cast<int>(trigger.Device));
				break;
			}

			return 0.0f;
		}
	}

	void InputActionManager::Evaluate()
	{
		for (int i = 0; i < m_Actions.size(); ++i)
		{
			InputActionState& state = m_ActionStates[i];
			const InputAction& action = m_Actions[i];
			bool wasDown = state.RawIsDown;
			state.IsDown = false;
			state.JustPressed = false;
			state.JustReleased = false;
			state.Strength = 0.0f;

			float actuation = 1.0f;
			for (const InputTrigger& trigger : action.Triggers)
			{
				const float strength = GetTriggerStrength(trigger);
				if (strength == 0.0f)
					continue;

				// The strongest trigger wins, so a stick and a key bound to the same action do not
				// fight over which one gets reported.
				if (Math::Abs(strength) <= Math::Abs(state.Strength))
					continue;

				state.Strength = strength;
				state.LastDevice = trigger.Device;
				actuation = GetTriggerActuation(trigger);
			}

			// Strength stays analog for whoever wants it; only the digital read is thresholded, so a
			// stick can report a gentle 0.2 lean without every direction claiming to be held down.
			state.RawIsDown = state.Strength != 0.0f && Math::Abs(state.Strength) >= actuation;

			// A consumed press stays swallowed until the control is physically released, and the
			// release goes with it - gameplay never saw the press, so the matching release is a phantom.
			if (state.Consumed)
			{
				state.Consumed = state.RawIsDown;
				state.Strength = 0.0f;
				continue;
			}

			state.IsDown = state.RawIsDown;
			state.JustPressed = state.IsDown && !wasDown;
			state.JustReleased = wasDown && !state.IsDown;
		}
	}

	void InputActionManager::ConsumeControl(InputDevice device, const InputControlId& control)
	{
		for (size_t i = 0; i < m_Actions.size(); ++i)
		{
			const InputAction& action = m_Actions[i];

			bool bound = std::any_of(action.Triggers.begin(), action.Triggers.end(),
				[&](const InputTrigger& trigger) { return trigger.Device == device && trigger.ControlId == control; });

			if (!bound)
				continue;

			// Evaluate already ran this frame, so this frame's reads are cleared here rather than there.
			InputActionState& state = m_ActionStates[i];
			state.Consumed = true;
			state.IsDown = false;
			state.JustPressed = false;
			state.JustReleased = false;
			state.Strength = 0.0f;
		}
	}

	bool InputActionManager::IsActionDown(std::string_view actionName)
	{
		int actionIndex = GetActionIndex(actionName);
		if (actionIndex == -1)
		{
			EB_CORE_ERROR("InputActionManager::IsActionDown: Action not found: {0}", actionName);
			return false;
		}

		return m_ActionStates[actionIndex].IsDown;
	}

	bool InputActionManager::IsActionPressed(std::string_view actionName)
	{
		int actionIndex = GetActionIndex(actionName);
		if (actionIndex == -1)
		{
			EB_CORE_ERROR("InputActionManager::IsActionPressed: Action not found: {0}", actionName);
			return false;
		}

		return m_ActionStates[actionIndex].JustPressed;
	}

	bool InputActionManager::IsActionReleased(std::string_view actionName)
	{
		int actionIndex = GetActionIndex(actionName);
		if (actionIndex == -1)
		{
			EB_CORE_ERROR("InputActionManager::IsActionReleased: Action not found: {0}", actionName);
			return false;
		}

		return m_ActionStates[actionIndex].JustReleased;
	}

	float InputActionManager::GetActionStrength(std::string_view actionName)
	{
		int actionIndex = GetActionIndex(actionName);
		if (actionIndex == -1)
		{
			EB_CORE_ERROR("InputActionManager::GetActionStrength: Action not found: {0}", actionName);
			return 0.0f;
		}
		return m_ActionStates[actionIndex].Strength;
	}

	float InputActionManager::GetAxis(std::string_view negative, std::string_view positive)
	{
		return GetActionStrength(positive) - GetActionStrength(negative);
	}

	Vector2f InputActionManager::GetAxis2D(std::string_view left, std::string_view right, std::string_view down, std::string_view up)
	{
		return Vector2f(GetAxis(left, right), GetAxis(down, up));
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
		int actionIndex = GetActionIndex(actionName);
		if (actionIndex == -1)
			return;

		RemoveAction(actionIndex);
	}

	void InputActionManager::AddTrigger(int actionIndex, const InputTrigger& trigger)
	{
		EB_CORE_ASSERT(actionIndex >= 0 && actionIndex < m_Actions.size(), "InputActionManager::AddTrigger: Action index out of range");
		m_Actions[actionIndex].Triggers.push_back(trigger);
		m_ActionStates[actionIndex] = InputActionState(); // Reset state for the action
	}

	void InputActionManager::AddTrigger(std::string_view actionName, const InputTrigger& trigger)
	{
		int actionIndex = GetActionIndex(actionName);
		if (actionIndex == -1)
			return;

		AddTrigger(actionIndex, trigger);
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
		EB_CORE_ASSERT(triggerIndex >= 0 && triggerIndex < m_Actions[actionIndex].Triggers.size(), "InputActionManager::RemoveTrigger: Trigger index out of range");
		m_Actions[actionIndex].Triggers.erase(m_Actions[actionIndex].Triggers.begin() + triggerIndex);
		m_ActionStates[actionIndex] = InputActionState(); // Reset state for the action
	}

	void InputActionManager::RemoveTrigger(std::string_view actionName, int triggerIndex)
	{
		int actionIndex = GetActionIndex(actionName);
		if (actionIndex == -1)
			return;

		RemoveTrigger(actionIndex, triggerIndex);
	}

	void InputActionManager::ClearActions()
	{
		m_Actions.clear();
		m_ActionStates.clear();
	}

}