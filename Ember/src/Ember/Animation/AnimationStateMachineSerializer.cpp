#include "ebpch.h"

#include "AnimationStateMachineSerializer.h"

#include <fstream>

namespace Ember {

	// Magic number for validation: "ASMS"
	const uint32_t ASMS_FILE_MAGIC = 0x41534D53;
	// File format versions:
	//   1 = Default state + states + transitions (legacy string conditions)
	//   2 = AnimationCondition payload per transition condition
	//   3 = States and transitions identified by UUID; default state stored as UUID
	//   4 = Transition UUID persisted for unique transition identity
	//   5 = Unified AnimationParameter map on ASM; int threshold added to conditions
	//   6 = ASM parameters store definitions only (name + AnimationParameterType)
	//   7 = ASM parameters store full AnimationParameter defaults again
	const uint32_t ASMS_FILE_VERSION = 7;

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
			file.write((const char*)&condition.IntValue, sizeof(int32_t));
		}

		void WriteParameter(std::ofstream& file, const std::string& name, const AnimationParameter& parameter)
		{
			WriteString(file, name);
			uint32_t type = static_cast<uint32_t>(parameter.Type);
			uint8_t boolValue = parameter.BoolValue ? 1u : 0u;
			file.write((const char*)&type, sizeof(uint32_t));
			file.write((const char*)&parameter.FloatValue, sizeof(float));
			file.write((const char*)&boolValue, sizeof(uint8_t));
			file.write((const char*)&parameter.IntValue, sizeof(int32_t));
		}

		bool ReadCondition(std::ifstream& file, AnimationCondition& condition, uint32_t fileVersion)
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

			if (fileVersion >= 5)
			{
				file.read((char*)&condition.IntValue, sizeof(int32_t));
				if (!file.good())
					return false;
			}

			condition.Type = static_cast<AnimationParameterType>(type);
			condition.Operator = static_cast<AnimationConditionOperator>(op);
			condition.BoolValue = boolValue != 0;
			return true;
		}

		bool ReadParameterWithValues(std::ifstream& file, std::string& name, AnimationParameter& parameter)
		{
			if (!ReadString(file, name))
				return false;

			uint32_t type = 0;
			uint8_t boolValue = 0;
			file.read((char*)&type, sizeof(uint32_t));
			file.read((char*)&parameter.FloatValue, sizeof(float));
			file.read((char*)&boolValue, sizeof(uint8_t));
			file.read((char*)&parameter.IntValue, sizeof(int32_t));
			if (!file.good())
				return false;

			parameter.Type = static_cast<AnimationParameterType>(type);
			parameter.BoolValue = boolValue != 0;
			return true;
		}

		bool ReadParameterV6(std::ifstream& file, std::string& name, AnimationParameter& parameter)
		{
			if (!ReadString(file, name))
				return false;

			uint32_t type = 0;
			file.read((char*)&type, sizeof(uint32_t));
			if (!file.good())
				return false;

			parameter.Type = static_cast<AnimationParameterType>(type);
			parameter.FloatValue = 0.0f;
			parameter.BoolValue = false;
			parameter.IntValue = 0;
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

		// Write default state UUID
		uint64_t defaultStateId = (uint64_t)animationStateMachine->GetDefaultState();
		file.write((const char*)&defaultStateId, sizeof(uint64_t));

		const auto& states = animationStateMachine->GetStates();
		uint32_t stateCount = static_cast<uint32_t>(states.size());
		file.write((const char*)&stateCount, sizeof(uint32_t));
		for (const auto& [stateId, state] : states)
		{
			uint64_t stateUUID = (uint64_t)state.Id;
			file.write((const char*)&stateUUID, sizeof(uint64_t));

			WriteString(file, state.Name);

			uint64_t animationHandle = (uint64_t)state.AnimationHandle;
			file.write((const char*)&animationHandle, sizeof(uint64_t));

			uint8_t looping = state.Looping ? 1u : 0u;
			file.write((const char*)&looping, sizeof(uint8_t));
			file.write((const char*)&state.BasePlaybackSpeed, sizeof(float));

			// Write node positions
			Vector2f nodePos = state.NodePosition;
			file.write((const char*)&nodePos, sizeof(Vector2f));
		}

		const auto& transitionsBySource = animationStateMachine->GetTransitions();
		uint32_t transitionCount = 0;
		for (const auto& [fromState, transitions] : transitionsBySource)
		{
			transitionCount += static_cast<uint32_t>(transitions.size());
		}
		file.write((const char*)&transitionCount, sizeof(uint32_t));

		for (const auto& [fromStateId, transitions] : transitionsBySource)
		{
			for (const auto& transition : transitions)
			{
				uint64_t transitionId = (uint64_t)transition.Id;
				uint64_t fromId = (uint64_t)transition.FromStateId;
				uint64_t toId = (uint64_t)transition.ToStateId;
				file.write((const char*)&transitionId, sizeof(uint64_t));
				file.write((const char*)&fromId, sizeof(uint64_t));
				file.write((const char*)&toId, sizeof(uint64_t));
				file.write((const char*)&transition.BlendDuration, sizeof(float));

				uint32_t conditionCount = static_cast<uint32_t>(transition.Conditions.size());
				file.write((const char*)&conditionCount, sizeof(uint32_t));
				for (const auto& condition : transition.Conditions)
				{
					WriteCondition(file, condition);
				}
			}
		}

		// Write entry/exit node positions
		file.write((const char*)&animationStateMachine->EntryNodePosition, sizeof(Vector2f));
		file.write((const char*)&animationStateMachine->ExitNodePosition, sizeof(Vector2f));

		// Write parameter definitions (v5+)
		const auto& parameters = animationStateMachine->GetParameters();
		uint32_t paramCount = static_cast<uint32_t>(parameters.size());
		file.write((const char*)&paramCount, sizeof(uint32_t));
		for (const auto& [name, parameter] : parameters)
		{
			WriteParameter(file, name, parameter);
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

		// Legacy (v1/v2): default state was stored as a name string; resolve to UUID after states are loaded.
		std::string legacyDefaultStateName;
		UUID defaultStateId = Constants::InvalidUUID;

		if (version < 3)
		{
			if (!ReadString(file, legacyDefaultStateName))
			{
				EB_CORE_ERROR("Failed reading default state from animation state machine file: {0}", filepath.string());
				return nullptr;
			}
		}
		else
		{
			uint64_t defaultId = 0;
			file.read((char*)&defaultId, sizeof(uint64_t));
			if (!file.good())
			{
				EB_CORE_ERROR("Failed reading default state UUID from animation state machine file: {0}", filepath.string());
				return nullptr;
			}
			defaultStateId = UUID(defaultId);
		}

		uint32_t stateCount = 0;
		file.read((char*)&stateCount, sizeof(uint32_t));
		if (!file.good())
		{
			EB_CORE_ERROR("Failed reading state count from animation state machine file: {0}", filepath.string());
			return nullptr;
		}

		// For legacy files: map state name -> generated UUID so transitions can reference them
		std::unordered_map<std::string, UUID> legacyNameToId;

		for (uint32_t i = 0; i < stateCount; i++)
		{
			AnimationState state;

			if (version >= 3)
			{
				uint64_t stateUUID = 0;
				file.read((char*)&stateUUID, sizeof(uint64_t));
				state.Id = UUID(stateUUID);
			}

			if (!ReadString(file, state.Name))
			{
				EB_CORE_ERROR("Failed reading state name from animation state machine file: {0}", filepath.string());
				return nullptr;
			}

			if (version < 3)
			{
				// Generate a stable UUID from the name hash for legacy files
				state.Id = UUID(std::hash<std::string>{}(state.Name));
				legacyNameToId[state.Name] = state.Id;
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

			file.read((char*)&state.NodePosition, sizeof(Vector2f));

			animationStateMachine->AddState(state);
		}

		// Resolve legacy default state name to UUID
		if (version < 3)
		{
			if (!legacyDefaultStateName.empty() && legacyNameToId.contains(legacyDefaultStateName))
				defaultStateId = legacyNameToId[legacyDefaultStateName];
		}
		animationStateMachine->SetDefaultState(defaultStateId);

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

			if (version >= 4)
			{
				uint64_t transitionId = 0, fromId = 0, toId = 0;
				file.read((char*)&transitionId, sizeof(uint64_t));
				file.read((char*)&fromId, sizeof(uint64_t));
				file.read((char*)&toId, sizeof(uint64_t));
				transition.Id = UUID(transitionId);
				transition.FromStateId = UUID(fromId);
				transition.ToStateId = UUID(toId);
			}
			else if (version >= 3)
			{
				uint64_t fromId = 0, toId = 0;
				file.read((char*)&fromId, sizeof(uint64_t));
				file.read((char*)&toId, sizeof(uint64_t));
				transition.FromStateId = UUID(fromId);
				transition.ToStateId = UUID(toId);
			}
			else
			{
				std::string fromName, toName;
				if (!ReadString(file, fromName) || !ReadString(file, toName))
				{
					EB_CORE_ERROR("Failed reading transition states from animation state machine file: {0}", filepath.string());
					return nullptr;
				}
				transition.FromStateId = legacyNameToId.contains(fromName) ? legacyNameToId[fromName] : UUID(Constants::InvalidUUID);
				transition.ToStateId = legacyNameToId.contains(toName) ? legacyNameToId[toName] : UUID(Constants::InvalidUUID);
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
					if (!ReadCondition(file, condition, version))
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

		// Read entry/exit node positions
		file.read((char*)&animationStateMachine->EntryNodePosition, sizeof(Vector2f));
		file.read((char*)&animationStateMachine->ExitNodePosition, sizeof(Vector2f));

		// Read parameter definitions/defaults (v5+)
		if (version >= 5)
		{
			uint32_t paramCount = 0;
			file.read((char*)&paramCount, sizeof(uint32_t));
			if (!file.good())
			{
				EB_CORE_ERROR("Failed reading parameter count from animation state machine file: {0}", filepath.string());
				return nullptr;
			}

			for (uint32_t i = 0; i < paramCount; i++)
			{
				std::string paramName;
				AnimationParameter parameter;
				bool readSuccess = false;
				if (version == 6)
					readSuccess = ReadParameterV6(file, paramName, parameter);
				else
					readSuccess = ReadParameterWithValues(file, paramName, parameter);

				if (!readSuccess)
				{
					EB_CORE_ERROR("Failed reading parameter from animation state machine file: {0}", filepath.string());
					return nullptr;
				}
				animationStateMachine->AddParameter(paramName, parameter);
			}
		}

		animationStateMachine->SetFilePath(filepath.string());
		return animationStateMachine;
	}

}
