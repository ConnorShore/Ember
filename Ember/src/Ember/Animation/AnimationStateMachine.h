#pragma once

#include "AnimationState.h"
#include "AnimationTransition.h"

#include "Ember/Asset/Asset.h"

#include <unordered_map>

namespace Ember {

	class AnimationStateMachine : public Asset
	{
	public:
		AnimationStateMachine(UUID uuid, const std::string& name, const std::string& filePath)
			: Asset(uuid, name, filePath, AssetType::AnimationStateMachine) { }
		AnimationStateMachine(const std::string& name, const std::string& filePath)
			: AnimationStateMachine(UUID(), name, filePath) { }

		void AddState(const AnimationState& state)
		{
			m_States[state.Name] = state;
		}

		void AddTransition(const AnimationTransition& transition)
		{
			m_Transitions[transition.FromStateName].push_back(transition);
		}

		const std::string& GetDefaultState() const { return m_DefaultState; }
		void SetDefaultState(const std::string& defaultState) { m_DefaultState = defaultState; }

		const std::unordered_map<std::string, AnimationState>& GetStates() const { return m_States; }
		const std::unordered_map<std::string, std::vector<AnimationTransition>>& GetTransitions() const { return m_Transitions; }

		static AssetType GetStaticType() { return AssetType::AnimationStateMachine; }
	
	private:
		std::string m_DefaultState; 
		std::unordered_map<std::string, AnimationState> m_States;
		std::unordered_map<std::string, std::vector<AnimationTransition>> m_Transitions;
	};

}