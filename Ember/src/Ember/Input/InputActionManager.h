#pragma once

#include "InputAction.h"
#include "InputActionState.h"

#include <vector>

namespace Ember {

	class InputActionManager
	{
	public:
		void Evaluate();

		bool IsActionDown(std::string_view actionName);
		bool IsActionPressed(std::string_view actionName);
		bool IsActionReleased(std::string_view actionName);

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