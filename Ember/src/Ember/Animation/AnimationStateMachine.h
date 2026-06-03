#pragma once

#include "AnimationState.h"
#include "AnimationTransition.h"

#include "Ember/Asset/Asset.h"
#include "Ember/Core/Constants.h"

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
			m_States[state.Id] = state;
		}

		void AddTransition(const AnimationTransition& transition)
		{
			m_Transitions[transition.FromStateId].push_back(transition);
		}

		const UUID& GetDefaultState() const { return m_DefaultState; }
		void SetDefaultState(const UUID& defaultState) { m_DefaultState = defaultState; }

		const std::unordered_map<UUID, AnimationState>& GetStates() const { return m_States; }
		const std::unordered_map<UUID, std::vector<AnimationTransition>>& GetTransitions() const { return m_Transitions; }

		static AssetType GetStaticType() { return AssetType::AnimationStateMachine; }

	private:
		UUID m_DefaultState = Constants::InvalidUUID;
		std::unordered_map<UUID, AnimationState> m_States;
		std::unordered_map<UUID, std::vector<AnimationTransition>> m_Transitions;
	};

}