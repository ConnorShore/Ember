#include "ebpch.h"

#include "AnimationStateMachineSerializer.h"

#include <fstream>

namespace Ember {

	// Magic number for validation: "ASMS"
	const uint32_t ASMS_FILE_MAGIC = 0x41534D53;
	// File format versions:
	//   1 = Default state + states + transitions (legacy string conditions)
	//   2 = AnimationCondition payload per transition condition
	const uint32_t ASMS_FILE_VERSION = 2;

	namespace
	{
		void WriteString(std::ofstream& file, const std::string& value)
		{
			uint32_t length = static_cast<uint32_t>(value.size());
			file.write((const char*)&length, sizeof(uint32_t));
			if (length > 0)
				file.write(value.data(), length);
		}

		bool ReadString(std::ifstream& file, std::string& value)
		{
			uint32_t length = 0;
			file.read((char*)&length, sizeof(uint32_t));
			if (!file.good())
				return false;

			value.resize(length);
			if (length > 0)
				file.read(value.data(), length);

			return file.good();
		}

		void WriteCondition(std::ofstream& file, const AnimationCondition& condition)
		{
			WriteString(file, condition.ParameterName);

			uint32_t type = static_cast<uint32_t>(condition.Type);
			uint32_t op = static_cast<uint32_t>(condition.Operator);
			uint8_t boolValue = condition.BoolValue ? 1u : 0u;

			file.write((const char*)&type, sizeof(uint32_t));
			file.write((const char*)&op, sizeof(uint32_t));
			file.write((const char*)&condition.FloatValue, sizeof(float));
			file.write((const char*)&boolValue, sizeof(uint8_t));
		}

		bool ReadCondition(std::ifstream& file, AnimationCondition& condition)
		{
			if (!ReadString(file, condition.ParameterName))
				return false;

			uint32_t type = 0;
			uint32_t op = 0;
			uint8_t boolValue = 0;

			file.read((char*)&type, sizeof(uint32_t));
			file.read((char*)&op, sizeof(uint32_t));
			file.read((char*)&condition.FloatValue, sizeof(float));
			file.read((char*)&boolValue, sizeof(uint8_t));
			if (!file.good())
				return false;

			condition.Type = static_cast<AnimationParameterType>(type);
			condition.Operator = static_cast<AnimationConditionOperator>(op);
			condition.BoolValue = boolValue != 0;
			return true;
		}
	}

	bool AnimationStateMachineSerializer::Serialize(const std::filesystem::path& filepath, const SharedPtr<AnimationStateMachine>& animationStateMachine)
	{
		std::ofstream file(filepath, std::ios::binary | std::ios::trunc);
		if (!file.is_open())
			return false;

		uint32_t magic = ASMS_FILE_MAGIC;
		uint32_t version = ASMS_FILE_VERSION;
		file.write((const char*)&magic, sizeof(uint32_t));
		file.write((const char*)&version, sizeof(uint32_t));

		WriteString(file, animationStateMachine->GetDefaultState());

		const auto& states = animationStateMachine->GetStates();
		uint32_t stateCount = static_cast<uint32_t>(states.size());
		file.write((const char*)&stateCount, sizeof(uint32_t));
		for (const auto& [stateName, state] : states)
		{
			WriteString(file, state.Name);

			uint64_t animationHandle = (uint64_t)state.AnimationHandle;
			file.write((const char*)&animationHandle, sizeof(uint64_t));

			uint8_t looping = state.Looping ? 1u : 0u;
			file.write((const char*)&looping, sizeof(uint8_t));
			file.write((const char*)&state.BasePlaybackSpeed, sizeof(float));
		}

		const auto& transitionsBySource = animationStateMachine->GetTransitions();
		uint32_t transitionCount = 0;
		for (const auto& [fromState, transitions] : transitionsBySource)
		{
			transitionCount += static_cast<uint32_t>(transitions.size());
		}
		file.write((const char*)&transitionCount, sizeof(uint32_t));

		for (const auto& [fromState, transitions] : transitionsBySource)
		{
			for (const auto& transition : transitions)
			{
				WriteString(file, transition.FromStateName);
				WriteString(file, transition.ToStateName);
				file.write((const char*)&transition.BlendDuration, sizeof(float));

				uint32_t conditionCount = static_cast<uint32_t>(transition.Conditions.size());
				file.write((const char*)&conditionCount, sizeof(uint32_t));
				for (const auto& condition : transition.Conditions)
				{
					WriteCondition(file, condition);
				}
			}
		}

		file.close();
		return true;
	}

	SharedPtr<AnimationStateMachine> AnimationStateMachineSerializer::Deserialize(UUID uuid, const std::filesystem::path& filepath)
	{
		std::ifstream file(filepath, std::ios::binary);
		if (!file.is_open())
		{
			EB_CORE_ERROR("Failed to open animation state machine file: {0}", filepath.string());
			return nullptr;
		}

		uint32_t magic = 0;
		uint32_t version = 0;
		file.read((char*)&magic, sizeof(uint32_t));
		file.read((char*)&version, sizeof(uint32_t));

		if (magic != ASMS_FILE_MAGIC)
		{
			EB_CORE_ERROR("Invalid animation state machine file magic in: {0}", filepath.string());
			return nullptr;
		}

		if (version > ASMS_FILE_VERSION)
		{
			EB_CORE_ERROR("Animation state machine file version {0} is newer than supported version {1}: {2}", version, ASMS_FILE_VERSION, filepath.string());
			return nullptr;
		}

		std::string name = filepath.stem().string();
		auto animationStateMachine = SharedPtr<AnimationStateMachine>::Create(uuid, name, filepath.string());

		std::string defaultState;
		if (!ReadString(file, defaultState))
		{
			EB_CORE_ERROR("Failed reading default state from animation state machine file: {0}", filepath.string());
			return nullptr;
		}
		animationStateMachine->SetDefaultState(defaultState);

		uint32_t stateCount = 0;
		file.read((char*)&stateCount, sizeof(uint32_t));
		if (!file.good())
		{
			EB_CORE_ERROR("Failed reading state count from animation state machine file: {0}", filepath.string());
			return nullptr;
		}

		for (uint32_t i = 0; i < stateCount; i++)
		{
			AnimationState state;
			if (!ReadString(file, state.Name))
			{
				EB_CORE_ERROR("Failed reading state name from animation state machine file: {0}", filepath.string());
				return nullptr;
			}

			uint64_t animationHandle = 0;
			file.read((char*)&animationHandle, sizeof(uint64_t));
			state.AnimationHandle = UUID(animationHandle);

			uint8_t looping = 0;
			file.read((char*)&looping, sizeof(uint8_t));
			state.Looping = looping != 0;

			file.read((char*)&state.BasePlaybackSpeed, sizeof(float));
			if (!file.good())
			{
				EB_CORE_ERROR("Failed reading state payload from animation state machine file: {0}", filepath.string());
				return nullptr;
			}

			animationStateMachine->AddState(state);
		}

		uint32_t transitionCount = 0;
		file.read((char*)&transitionCount, sizeof(uint32_t));
		if (!file.good())
		{
			EB_CORE_ERROR("Failed reading transition count from animation state machine file: {0}", filepath.string());
			return nullptr;
		}

		for (uint32_t i = 0; i < transitionCount; i++)
		{
			AnimationTransition transition;
			if (!ReadString(file, transition.FromStateName) || !ReadString(file, transition.ToStateName))
			{
				EB_CORE_ERROR("Failed reading transition states from animation state machine file: {0}", filepath.string());
				return nullptr;
			}

			file.read((char*)&transition.BlendDuration, sizeof(float));

			uint32_t conditionCount = 0;
			file.read((char*)&conditionCount, sizeof(uint32_t));
			if (!file.good())
			{
				EB_CORE_ERROR("Failed reading transition metadata from animation state machine file: {0}", filepath.string());
				return nullptr;
			}

			transition.Conditions.reserve(conditionCount);
			for (uint32_t c = 0; c < conditionCount; c++)
			{
				AnimationCondition condition;
				if (version >= 2)
				{
					if (!ReadCondition(file, condition))
					{
						EB_CORE_ERROR("Failed reading transition condition from animation state machine file: {0}", filepath.string());
						return nullptr;
					}
				}
				else
				{
					std::string legacyCondition;
					if (!ReadString(file, legacyCondition))
					{
						EB_CORE_ERROR("Failed reading transition condition from animation state machine file: {0}", filepath.string());
						return nullptr;
					}

					condition.ParameterName = legacyCondition;
					condition.Type = AnimationParameterType::Trigger;
					condition.Operator = AnimationConditionOperator::Equal;
					condition.BoolValue = true;
				}

				transition.Conditions.push_back(condition);
			}

			animationStateMachine->AddTransition(transition);
		}

		animationStateMachine->SetFilePath(filepath.string());
		return animationStateMachine;
	}

}
