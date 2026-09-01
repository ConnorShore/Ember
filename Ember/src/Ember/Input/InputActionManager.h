#pragma once

#include "InputAction.h"
#include "InputActionState.h"

#include <vector>

namespace Ember {

	class InputActionManager
	{
	public:
		void Evaluate();

		// Swallows a press the UI has already acted on, so it cannot also drive gameplay. Every
		// system that polls actions runs after UIInputSystem, and Evaluate happens once at the top
		// of the frame, so without this one button press is consumed twice in the same frame.
		void ConsumeControl(InputDevice device, const InputControlId& control);

		// The same swallow by action name, silencing whichever control is actuating it right now - so
		// every other action bound to that same button goes quiet with it.
		void ConsumeAction(std::string_view actionName);

		bool IsActionDown(std::string_view actionName);
		bool IsActionPressed(std::string_view actionName);
		bool IsActionReleased(std::string_view actionName);

		float GetActionStrength(std::string_view actionName);

		float GetAxis(std::string_view negative, std::string_view positive);
		Vector2f GetAxis2D(std::string_view left, std::string_view right, std::string_view down, std::string_view up);

		int GetActionIndex(std::string_view actionName);

		void AddAction(const InputAction& action);
		void EditActionName(int index, std::string_view newName);
		void RemoveAction(std::string_view actionName);
		void RemoveAction(int index);

		void AddTrigger(int actionIndex, const InputTrigger& trigger);
		void AddTrigger(std::string_view actionName, const InputTrigger& trigger);
		void UpdateTrigger(int actionIndex, int triggerIndex, const InputTrigger& trigger);
		void RemoveTrigger(int actionIndex, int triggerIndex);
		void RemoveTrigger(std::string_view actionName, int triggerIndex);

		void ClearActions();
		
		inline const std::vector<InputAction>& GetActions() { return m_Actions; }
		inline const std::vector<InputActionState>& GetActionStates() { return m_ActionStates; }

	private:
		std::vector<InputAction> m_Actions;
		std::vector<InputActionState> m_ActionStates;
	};

}