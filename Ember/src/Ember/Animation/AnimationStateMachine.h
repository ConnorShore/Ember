#pragma once

#include "AnimationState.h"
#include "AnimationTransition.h"
#include "AnimationParameter.h"

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

		inline AnimationState& CreateState(const std::string& stateName)
		{
			AnimationState state(stateName);
			m_States[state.Id] = state;
			return m_States[state.Id];
		}

		inline void AddState(const AnimationState& state)
		{
			m_States[state.Id] = state;
		}

		inline void RemoveState(UUID stateId)
		{
			m_States.erase(stateId);
			m_Transitions.erase(stateId);
			for (auto& [fromStateId, transitions] : m_Transitions)
			{
				transitions.erase(
					std::remove_if(transitions.begin(), transitions.end(),
						[&stateId](const AnimationTransition& t) { return t.ToStateId == stateId; }),
					transitions.end());
			}
		}

		inline AnimationTransition& CreateTransition(UUID fromStateId, UUID toStateId)
		{
			AnimationTransition transition;
			transition.Id = UUID();
			transition.FromStateId = fromStateId;
			transition.ToStateId = toStateId;
			m_Transitions[fromStateId].push_back(transition);
			return m_Transitions[fromStateId].back();
		}

		inline void AddTransition(const AnimationTransition& transition)
		{
			m_Transitions[transition.FromStateId].push_back(transition);
		}

		inline void RemoveTransition(UUID transitionId)
		{
			for (auto& [fromStateId, transitions] : m_Transitions)
			{
				transitions.erase(
					std::remove_if(transitions.begin(), transitions.end(),
						[&transitionId](const AnimationTransition& t) { return t.Id == transitionId; }),
					transitions.end());
			}

			// Clean up any empty transition lists
			for (auto it = m_Transitions.begin(); it != m_Transitions.end(); )
			{
				if (it->second.empty())
					it = m_Transitions.erase(it);
				else
					++it;
			}
		}

		inline void RemoveTransition(UUID fromStateId, UUID toStateId)
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

		inline const UUID& GetDefaultState() const { return m_DefaultState; }
		inline void SetDefaultState(const UUID& defaultState) { m_DefaultState = defaultState; }

		inline const bool ContainsState(const std::string& stateName) const
		{
			for (const auto& [id, state] : m_States)
			{
				if (state.Name == stateName)
					return true;
			}
			return false;
		}

		inline std::unordered_map<UUID, AnimationState>& GetStates() { return m_States; }
		inline const std::unordered_map<UUID, AnimationState>& GetStates() const { return m_States; }
		inline std::unordered_map<UUID, std::vector<AnimationTransition>>& GetTransitions() { return m_Transitions; }
		inline const std::unordered_map<UUID, std::vector<AnimationTransition>>& GetTransitions() const { return m_Transitions; }

		inline AnimationTransition* GetTransitionById(UUID transitionId)
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

		inline std::unordered_map<std::string, AnimationParameter>& GetParameters() { return m_Parameters; }
		inline const std::unordered_map<std::string, AnimationParameter>& GetParameters() const { return m_Parameters; }

		inline void AddParameter(const std::string& name, AnimationParameterType paramType)
		{
			AnimationParameter parameter;
			parameter.Type = paramType;
			m_Parameters[name] = parameter;
		}

		inline void AddParameter(const std::string& name, const AnimationParameter& parameter)
		{
			m_Parameters[name] = parameter;
		}

		inline void RemoveParameter(const std::string& name)
		{
			m_Parameters.erase(name);
		}

		inline static AssetType GetStaticType() { return AssetType::AnimationStateMachine; }

	public:
		Vector2f EntryNodePosition = Vector2f(0.0f);
		Vector2f ExitNodePosition = Vector2f(0.0f);
		bool EntryNodePositionSet = false;
		bool ExitNodePositionSet = false;

	private:
		UUID m_DefaultState = Constants::InvalidUUID;
		std::unordered_map<UUID, AnimationState> m_States;
		std::unordered_map<UUID, std::vector<AnimationTransition>> m_Transitions;
		std::unordered_map<std::string, AnimationParameter> m_Parameters;
	};

}