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

		void RemoveTransition(UUID fromStateId, UUID toStateId)
		{
			auto it = m_Transitions.find(fromStateId);
			if (it == m_Transitions.end())
				return;

			auto& vec = it->second;
			vec.erase(
				std::remove_if(vec.begin(), vec.end(),
					[&toStateId](const AnimationTransition& t) { return t.ToStateId == toStateId; }),
				vec.end());

			if (vec.empty())
				m_Transitions.erase(it);
		}

		const UUID& GetDefaultState() const { return m_DefaultState; }
		void SetDefaultState(const UUID& defaultState) { m_DefaultState = defaultState; }

		std::unordered_map<UUID, AnimationState>& GetStates() { return m_States; }
		const std::unordered_map<UUID, AnimationState>& GetStates() const { return m_States; }
		std::unordered_map<UUID, std::vector<AnimationTransition>>& GetTransitions() { return m_Transitions; }
		const std::unordered_map<UUID, std::vector<AnimationTransition>>& GetTransitions() const { return m_Transitions; }

		AnimationTransition* GetTransitionById(UUID transitionId)
		{
			for (auto& [fromStateId, transitions] : m_Transitions)
			{
				for (auto& transition : transitions)
				{
					if (transition.Id == transitionId)
						return &transition;
				}
			}
			return nullptr;
		}

		static AssetType GetStaticType() { return AssetType::AnimationStateMachine; }

	public:
		Vector2f EntryNodePosition = Vector2f(0.0f);
		Vector2f ExitNodePosition = Vector2f(0.0f);
		bool EntryNodePositionSet = false;
		bool ExitNodePositionSet = false;

	private:
		UUID m_DefaultState = Constants::InvalidUUID;
		std::unordered_map<UUID, AnimationState> m_States;
		std::unordered_map<UUID, std::vector<AnimationTransition>> m_Transitions;
	};

}