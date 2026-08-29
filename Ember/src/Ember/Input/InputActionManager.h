#pragma once

#include "InputAction.h"
#include "InputActionState.h"

#include <vector>
#include <string_view>

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
		void ClearActions();
		
		inline const std::vector<InputAction>& GetActions() { return m_Actions; }
		inline const std::vector<InputActionState>& GetActionStates() { return m_ActionStates; }

	private:
		std::vector<InputAction> m_Actions;
		std::vector<InputActionState> m_ActionStates;
	};

}