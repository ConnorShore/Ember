#include "ebpch.h"

#include "AnimationControllerSerializer.h"
#include "Ember/Utils/SerializationUtils.h"

#include "Ember/Core/Application.h"

#include <ryml.hpp>
#include <ryml_std.hpp>

#include <fstream>
#include <sstream>

namespace {
	template<typename T>
	void WriteRaw(std::ofstream& stream, const T& value)
	{
		stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
	}

	template<typename T>
	bool ReadRaw(std::ifstream& stream, T& value)
	{
		stream.read(reinterpret_cast<char*>(&value), sizeof(T));
		return stream.good();
	}

	void WriteString(std::ofstream& stream, const std::string& value)
	{
		uint16_t len = static_cast<uint16_t>(value.size());
		WriteRaw(stream, len);
		if (len > 0)
			stream.write(value.data(), len);
	}

	bool ReadString(std::ifstream& stream, std::string& value)
	{
		uint16_t len = 0;
		if (!ReadRaw(stream, len))
			return false;

		value.resize(len);
		if (len > 0)
			stream.read(value.data(), len);

		return stream.good();
	}
}

namespace Ember {

	namespace
	{
		constexpr uint32_t EBCONTROLLER_SOURCE_FILE_VERSION = 1;
		constexpr uint32_t EBCONTROLLER_COOKED_MAGIC = 0x45424343; // 'EBCC'
		constexpr uint32_t EBCONTROLLER_COOKED_VERSION = 2;

		bool IsCookedPath(const std::filesystem::path& filepath)
		{
			return filepath.extension() == ".bin";
		}

		std::filesystem::path GetCookedPath(const std::filesystem::path& filepath)
		{
			auto cookedPath = filepath;
			cookedPath.replace_extension(".bin");
			return cookedPath;
		}

		void SerializeVector2f(ryml::NodeRef node, const Vector2f& value)
		{
			node |= ryml::SEQ | ryml::FLOW_SL;
			node.append_child() << value.x;
			node.append_child() << value.y;
		}

		void DeserializeVector2f(ryml::NodeRef node, Vector2f& value)
		{
			if (!node.is_seq() || node.num_children() != 2)
				return;

			node[0] >> value.x;
			node[1] >> value.y;
		}

		void SerializeStateMachine(ryml::NodeRef node, const AnimationStateMachine& stateMachine)
		{
			node |= ryml::MAP;
			node["DefaultState"] << (uint64_t)stateMachine.GetDefaultState();
			node["EntryNodePositionSet"] << stateMachine.EntryNodePositionSet;
			node["ExitNodePositionSet"] << stateMachine.ExitNodePositionSet;
			SerializeVector2f(node["EntryNodePosition"], stateMachine.EntryNodePosition);
			SerializeVector2f(node["ExitNodePosition"], stateMachine.ExitNodePosition);

			auto statesNode = node["States"];
			statesNode |= ryml::SEQ;
			for (const auto& [stateId, state] : stateMachine.GetStates())
			{
				auto stateNode = statesNode.append_child();
				stateNode |= ryml::MAP;
				stateNode["Id"] << (uint64_t)state.Id;
				stateNode["Name"] << state.Name;
				stateNode["AnimationHandle"] << (uint64_t)state.AnimationHandle;
				stateNode["Looping"] << state.Looping;
				stateNode["BasePlaybackSpeed"] << state.BasePlaybackSpeed;
				stateNode["PositionSet"] << state.PositionSet;
				SerializeVector2f(stateNode["NodePosition"], state.NodePosition);
			}

			auto transitionsNode = node["Transitions"];
			transitionsNode |= ryml::SEQ;
			for (const auto& [fromStateId, transitions] : stateMachine.GetTransitions())
			{
				for (const auto& transition : transitions)
				{
					auto transitionNode = transitionsNode.append_child();
					transitionNode |= ryml::MAP;
					transitionNode["Id"] << (uint64_t)transition.Id;
					transitionNode["FromStateId"] << (uint64_t)transition.FromStateId;
					transitionNode["ToStateId"] << (uint64_t)transition.ToStateId;
					transitionNode["BlendDuration"] << transition.BlendDuration;

					auto conditionsNode = transitionNode["Conditions"];
					conditionsNode |= ryml::SEQ;
					for (const auto& condition : transition.Conditions)
					{
						auto conditionNode = conditionsNode.append_child();
						conditionNode |= ryml::MAP;
						conditionNode["ParameterName"] << condition.ParameterName;
						conditionNode["Type"] << static_cast<uint32_t>(condition.Type);
						conditionNode["Operator"] << static_cast<uint32_t>(condition.Operator);
						conditionNode["FloatValue"] << condition.FloatValue;
						conditionNode["BoolValue"] << condition.BoolValue;
						conditionNode["IntValue"] << condition.IntValue;
					}
				}
			}
		}

		void DeserializeStateMachine(ryml::NodeRef node, AnimationStateMachine& stateMachine)
		{
			if (!node.valid())
				return;

			if (node.has_child("DefaultState"))
			{
				uint64_t defaultState = Constants::InvalidUUID;
				node["DefaultState"] >> defaultState;
				stateMachine.SetDefaultState(UUID(defaultState));
			}

			Util::ReadField(node, "EntryNodePositionSet", stateMachine.EntryNodePositionSet);
			Util::ReadField(node, "ExitNodePositionSet", stateMachine.ExitNodePositionSet);
			if (node.has_child("EntryNodePosition"))
				DeserializeVector2f(node["EntryNodePosition"], stateMachine.EntryNodePosition);
			if (node.has_child("ExitNodePosition"))
				DeserializeVector2f(node["ExitNodePosition"], stateMachine.ExitNodePosition);

			if (node.has_child("States"))
			{
				for (auto stateNode : node["States"].children())
				{
					AnimationState state;
					uint64_t id = Constants::InvalidUUID;
					uint64_t animationHandle = Constants::InvalidUUID;
					stateNode["Id"] >> id;
					stateNode["Name"] >> state.Name;
					stateNode["AnimationHandle"] >> animationHandle;
					Util::ReadField(stateNode, "Looping", state.Looping);
					Util::ReadField(stateNode, "BasePlaybackSpeed", state.BasePlaybackSpeed);
					Util::ReadField(stateNode, "PositionSet", state.PositionSet);
					if (stateNode.has_child("NodePosition"))
						DeserializeVector2f(stateNode["NodePosition"], state.NodePosition);

					state.Id = UUID(id);
					state.AnimationHandle = UUID(animationHandle);
					stateMachine.AddState(state);
				}
			}

			if (node.has_child("Transitions"))
			{
				for (auto transitionNode : node["Transitions"].children())
				{
					AnimationTransition transition;
					uint64_t transitionId = Constants::InvalidUUID;
					uint64_t fromStateId = Constants::InvalidUUID;
					uint64_t toStateId = Constants::InvalidUUID;
					transitionNode["Id"] >> transitionId;
					transitionNode["FromStateId"] >> fromStateId;
					transitionNode["ToStateId"] >> toStateId;
					Util::ReadField(transitionNode, "BlendDuration", transition.BlendDuration);

					transition.Id = UUID(transitionId);
					transition.FromStateId = UUID(fromStateId);
					transition.ToStateId = UUID(toStateId);

					if (transitionNode.has_child("Conditions"))
					{
						for (auto conditionNode : transitionNode["Conditions"].children())
						{
							AnimationCondition condition;
							uint32_t type = static_cast<uint32_t>(AnimationParameterType::Trigger);
							uint32_t op = static_cast<uint32_t>(AnimationConditionOperator::Equal);

							conditionNode["ParameterName"] >> condition.ParameterName;
							Util::ReadField(conditionNode, "Type", type);
							Util::ReadField(conditionNode, "Operator", op);
							Util::ReadField(conditionNode, "FloatValue", condition.FloatValue);
							Util::ReadField(conditionNode, "BoolValue", condition.BoolValue);
							Util::ReadField(conditionNode, "IntValue", condition.IntValue);

							condition.Type = static_cast<AnimationParameterType>(type);
							condition.Operator = static_cast<AnimationConditionOperator>(op);
							transition.Conditions.push_back(condition);
						}
					}

					stateMachine.AddTransition(transition);
				}
			}
		}
	}

	void AnimationControllerSerializer::SetRuntimeLoadTier(RuntimeAssetLoadTier tier)
	{
		AssetSerializationMode::SetRuntimeLoadTier(tier);
	}

	RuntimeAssetLoadTier AnimationControllerSerializer::GetRuntimeLoadTier()
	{
		return AssetSerializationMode::GetRuntimeLoadTier();
	}

	bool AnimationControllerSerializer::SerializeSource(const std::filesystem::path& filepath, const SharedPtr<AnimationController>& animationController)
	{
		if (!animationController)
			return false;

		auto outputPath = filepath;
		outputPath.replace_extension(".ebcontroller");

		ryml::Tree tree;
		auto root = tree.rootref();
		root |= ryml::MAP;

		root["Version"] << EBCONTROLLER_SOURCE_FILE_VERSION;

		auto parametersNode = root["Parameters"];
		parametersNode |= ryml::SEQ;
		for (const auto& [name, parameter] : animationController->GetParameters())
		{
			auto parameterNode = parametersNode.append_child();
			parameterNode |= ryml::MAP;
			parameterNode["Name"] << name;
			parameterNode["Type"] << static_cast<uint32_t>(parameter.Type);
			parameterNode["FloatValue"] << parameter.FloatValue;
			parameterNode["BoolValue"] << parameter.BoolValue;
			parameterNode["IntValue"] << parameter.IntValue;
		}

		auto layersNode = root["Layers"];
		layersNode |= ryml::SEQ;
		for (const auto& layer : animationController->GetLayers())
		{
			auto layerNode = layersNode.append_child();
			layerNode |= ryml::MAP;
			layerNode["Name"] << layer.Name;
			layerNode["Weight"] << layer.Weight;
			layerNode["MaskHandle"] << (uint64_t)layer.MaskHandle;
			layerNode["Mode"] << static_cast<uint32_t>(layer.Mode);
			SerializeStateMachine(layerNode["StateMachine"], layer.StateMachine);
		}

		std::ofstream fout(outputPath.string(), std::ios::trunc);
		if (!fout.is_open())
			return false;

		fout << tree;
		fout.close();

		animationController->SetFilePath(outputPath.string());
		return true;
	}

	SharedPtr<AnimationController> AnimationControllerSerializer::DeserializeSource(UUID uuid, const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		if (!stream.is_open())
		{
			EB_CORE_ERROR("Failed to open animation controller file: {0}", filepath.string());
			return nullptr;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();
		std::string yamlData = strStream.str();

		ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(yamlData));
		auto root = tree.rootref();

		auto animationController = SharedPtr<AnimationController>::Create(uuid, filepath.stem().string(), filepath.string());

		if (root.has_child("Parameters"))
		{
			auto& parameters = animationController->GetParameters();
			for (auto parameterNode : root["Parameters"].children())
			{
				std::string name;
				uint32_t type = static_cast<uint32_t>(AnimationParameterType::Float);
				AnimationParameter parameter;

				parameterNode["Name"] >> name;
				Util::ReadField(parameterNode, "Type", type);
				Util::ReadField(parameterNode, "FloatValue", parameter.FloatValue);
				Util::ReadField(parameterNode, "BoolValue", parameter.BoolValue);
				Util::ReadField(parameterNode, "IntValue", parameter.IntValue);

				parameter.Type = static_cast<AnimationParameterType>(type);
				parameters[name] = parameter;
			}
		}

		if (root.has_child("Layers"))
		{
			auto& layers = animationController->GetLayers();
			for (auto layerNode : root["Layers"].children())
			{
				AnimationLayer layer;
				uint64_t maskHandle = Constants::InvalidUUID;

				Util::ReadField(layerNode, "Name", layer.Name);
				Util::ReadField(layerNode, "Weight", layer.Weight);
				Util::ReadField(layerNode, "MaskHandle", maskHandle);
				if (layerNode.has_child("Mode"))
				{
					uint32_t mode = 0;
					layerNode["Mode"] >> mode;
					layer.Mode = static_cast<AnimationLayerMode>(mode);
				}

				layer.MaskHandle = UUID(maskHandle);
				DeserializeStateMachine(layerNode["StateMachine"], layer.StateMachine);
				layers.push_back(std::move(layer));
			}
		}

		return animationController;
	}

	bool AnimationControllerSerializer::SerializeCooked(const std::filesystem::path& filepath, const SharedPtr<AnimationController>& animationController)
	{
		if (!animationController)
			return false;

		auto outputPath = GetCookedPath(filepath);
		std::ofstream stream(outputPath, std::ios::binary | std::ios::trunc);
		if (!stream.is_open())
			return false;

		WriteRaw(stream, EBCONTROLLER_COOKED_MAGIC);
		WriteRaw(stream, EBCONTROLLER_COOKED_VERSION);

		const auto& parameters = animationController->GetParameters();
		uint32_t parameterCount = static_cast<uint32_t>(parameters.size());
		WriteRaw(stream, parameterCount);
		for (const auto& [name, parameter] : parameters)
		{
			WriteString(stream, name);

			uint8_t type = static_cast<uint8_t>(parameter.Type);
			uint8_t boolValue = parameter.BoolValue ? 1 : 0;
			WriteRaw(stream, type);
			WriteRaw(stream, parameter.FloatValue);
			WriteRaw(stream, boolValue);
			WriteRaw(stream, parameter.IntValue);
		}

		const auto& layers = animationController->GetLayers();
		uint32_t layerCount = static_cast<uint32_t>(layers.size());
		WriteRaw(stream, layerCount);
		for (const auto& layer : layers)
		{
			WriteString(stream, layer.Name);
			WriteRaw(stream, layer.Weight);
			uint64_t maskHandle = static_cast<uint64_t>(layer.MaskHandle);
			WriteRaw(stream, maskHandle);
			uint8_t layerMode = static_cast<uint8_t>(layer.Mode);
			WriteRaw(stream, layerMode);

			const auto& stateMachine = layer.StateMachine;
			uint64_t defaultState = static_cast<uint64_t>(stateMachine.GetDefaultState());
			WriteRaw(stream, defaultState);

			const auto& states = stateMachine.GetStates();
			uint32_t stateCount = static_cast<uint32_t>(states.size());
			WriteRaw(stream, stateCount);
			for (const auto& [stateId, state] : states)
			{
				WriteRaw(stream, static_cast<uint64_t>(state.Id));
				WriteString(stream, state.Name);
				WriteRaw(stream, static_cast<uint64_t>(state.AnimationHandle));

				uint8_t looping = state.Looping ? 1 : 0;
				WriteRaw(stream, looping);
				WriteRaw(stream, state.BasePlaybackSpeed);
			}

			uint32_t transitionCount = 0;
			for (const auto& [fromStateId, transitions] : stateMachine.GetTransitions())
				transitionCount += static_cast<uint32_t>(transitions.size());

			WriteRaw(stream, transitionCount);
			for (const auto& [fromStateId, transitions] : stateMachine.GetTransitions())
			{
				for (const auto& transition : transitions)
				{
					WriteRaw(stream, static_cast<uint64_t>(transition.Id));
					WriteRaw(stream, static_cast<uint64_t>(transition.FromStateId));
					WriteRaw(stream, static_cast<uint64_t>(transition.ToStateId));
					WriteRaw(stream, transition.BlendDuration);

					uint32_t conditionCount = static_cast<uint32_t>(transition.Conditions.size());
					WriteRaw(stream, conditionCount);
					for (const auto& condition : transition.Conditions)
					{
						WriteString(stream, condition.ParameterName);

						uint8_t type = static_cast<uint8_t>(condition.Type);
						uint8_t op = static_cast<uint8_t>(condition.Operator);
						uint8_t boolValue = condition.BoolValue ? 1 : 0;
						WriteRaw(stream, type);
						WriteRaw(stream, op);
						WriteRaw(stream, condition.FloatValue);
						WriteRaw(stream, boolValue);
						WriteRaw(stream, condition.IntValue);
					}
				}
			}
		}

		stream.close();
		return true;
	}

	SharedPtr<AnimationController> AnimationControllerSerializer::DeserializeCooked(UUID uuid, const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath, std::ios::binary);
		if (!stream.is_open())
		{
			EB_CORE_ERROR("Failed to open cooked animation controller file: {0}", filepath.string());
			return nullptr;
		}

		uint32_t magic = 0;
		uint32_t version = 0;
		if (!ReadRaw(stream, magic) || !ReadRaw(stream, version))
		{
			EB_CORE_ERROR("Failed reading cooked animation controller header: {0}", filepath.string());
			return nullptr;
		}

		if (magic != EBCONTROLLER_COOKED_MAGIC)
		{
			EB_CORE_ERROR("Invalid cooked animation controller magic: {0}", filepath.string());
			return nullptr;
		}

		if (version > EBCONTROLLER_COOKED_VERSION)
		{
			EB_CORE_ERROR("Unsupported cooked animation controller version {0} in {1}", version, filepath.string());
			return nullptr;
		}

		auto animationController = SharedPtr<AnimationController>::Create(uuid, filepath.stem().string(), filepath.string());

		uint32_t parameterCount = 0;
		if (!ReadRaw(stream, parameterCount))
			return nullptr;

		auto& parameters = animationController->GetParameters();
		for (uint32_t i = 0; i < parameterCount; ++i)
		{
			std::string name;
			if (!ReadString(stream, name))
				return nullptr;

			AnimationParameter parameter;
			uint8_t type = 0;
			uint8_t boolValue = 0;
			if (!ReadRaw(stream, type) || !ReadRaw(stream, parameter.FloatValue) || !ReadRaw(stream, boolValue) || !ReadRaw(stream, parameter.IntValue))
				return nullptr;

			parameter.Type = static_cast<AnimationParameterType>(type);
			parameter.BoolValue = (boolValue != 0);
			parameters[name] = parameter;
		}

		uint32_t layerCount = 0;
		if (!ReadRaw(stream, layerCount))
			return nullptr;

		auto& layers = animationController->GetLayers();
		layers.reserve(layerCount);
		for (uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex)
		{
			AnimationLayer layer;
			if (!ReadString(stream, layer.Name))
				return nullptr;

			uint64_t maskHandle = Constants::InvalidUUID;
			if (!ReadRaw(stream, layer.Weight) || !ReadRaw(stream, maskHandle))
				return nullptr;
			layer.MaskHandle = UUID(maskHandle);

			if (version >= 2)
			{
				uint8_t layerMode = 0;
				if (!ReadRaw(stream, layerMode))
					return nullptr;
				layer.Mode = static_cast<AnimationLayerMode>(layerMode);
			}

			uint64_t defaultState = Constants::InvalidUUID;
			if (!ReadRaw(stream, defaultState))
				return nullptr;
			layer.StateMachine.SetDefaultState(UUID(defaultState));

			uint32_t stateCount = 0;
			if (!ReadRaw(stream, stateCount))
				return nullptr;

			for (uint32_t stateIndex = 0; stateIndex < stateCount; ++stateIndex)
			{
				AnimationState state;
				uint64_t stateId = Constants::InvalidUUID;
				uint64_t animationHandle = Constants::InvalidUUID;
				uint8_t looping = 0;

				if (!ReadRaw(stream, stateId) || !ReadString(stream, state.Name) || !ReadRaw(stream, animationHandle) || !ReadRaw(stream, looping) || !ReadRaw(stream, state.BasePlaybackSpeed))
					return nullptr;

				state.Id = UUID(stateId);
				state.AnimationHandle = UUID(animationHandle);
				state.Looping = (looping != 0);
				state.PositionSet = false;
				state.NodePosition = Vector2f(0.0f);
				layer.StateMachine.AddState(state);
			}

			uint32_t transitionCount = 0;
			if (!ReadRaw(stream, transitionCount))
				return nullptr;

			for (uint32_t transitionIndex = 0; transitionIndex < transitionCount; ++transitionIndex)
			{
				AnimationTransition transition;
				uint64_t transitionId = Constants::InvalidUUID;
				uint64_t fromStateId = Constants::InvalidUUID;
				uint64_t toStateId = Constants::InvalidUUID;

				if (!ReadRaw(stream, transitionId) || !ReadRaw(stream, fromStateId) || !ReadRaw(stream, toStateId) || !ReadRaw(stream, transition.BlendDuration))
					return nullptr;

				transition.Id = UUID(transitionId);
				transition.FromStateId = UUID(fromStateId);
				transition.ToStateId = UUID(toStateId);

				uint32_t conditionCount = 0;
				if (!ReadRaw(stream, conditionCount))
					return nullptr;

				transition.Conditions.reserve(conditionCount);
				for (uint32_t conditionIndex = 0; conditionIndex < conditionCount; ++conditionIndex)
				{
					AnimationCondition condition;
					if (!ReadString(stream, condition.ParameterName))
						return nullptr;

					uint8_t type = 0;
					uint8_t op = 0;
					uint8_t boolValue = 0;
					if (!ReadRaw(stream, type) || !ReadRaw(stream, op) || !ReadRaw(stream, condition.FloatValue) || !ReadRaw(stream, boolValue) || !ReadRaw(stream, condition.IntValue))
						return nullptr;

					condition.Type = static_cast<AnimationParameterType>(type);
					condition.Operator = static_cast<AnimationConditionOperator>(op);
					condition.BoolValue = (boolValue != 0);
					transition.Conditions.push_back(condition);
				}

				layer.StateMachine.AddTransition(transition);
			}

			layers.push_back(std::move(layer));
		}

		return animationController;
	}

	bool AnimationControllerSerializer::Serialize(const std::filesystem::path& filepath, const SharedPtr<AnimationController>& animationController)
	{
		return SerializeSource(filepath, animationController);
	}

	SharedPtr<AnimationController> AnimationControllerSerializer::Deserialize(UUID uuid, const std::filesystem::path& filepath)
	{
		const RuntimeAssetLoadTier runtimeTier = GetRuntimeLoadTier();

		if (runtimeTier == RuntimeAssetLoadTier::ForceSourceYaml)
			return DeserializeSource(uuid, filepath);

		if (runtimeTier == RuntimeAssetLoadTier::ForceCookedBinary)
		{
			if (auto cooked = DeserializeCooked(uuid, GetCookedPath(filepath)))
				return cooked;
			return DeserializeSource(uuid, filepath);
		}

		if (IsCookedPath(filepath))
		{
			if (auto cooked = DeserializeCooked(uuid, filepath))
				return cooked;
		}

		return DeserializeSource(uuid, filepath);
	}

}
