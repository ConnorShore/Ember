#include "ebpch.h"

#include "AnimationControllerSerializer.h"

#include <ryml.hpp>
#include <ryml_std.hpp>

#include <fstream>
#include <sstream>

namespace Ember {

	namespace
	{
		constexpr uint32_t EBCONTROLLER_FILE_VERSION = 1;

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

			if (node.has_child("EntryNodePositionSet"))
				node["EntryNodePositionSet"] >> stateMachine.EntryNodePositionSet;
			if (node.has_child("ExitNodePositionSet"))
				node["ExitNodePositionSet"] >> stateMachine.ExitNodePositionSet;
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
					if (stateNode.has_child("Looping"))
						stateNode["Looping"] >> state.Looping;
					if (stateNode.has_child("BasePlaybackSpeed"))
						stateNode["BasePlaybackSpeed"] >> state.BasePlaybackSpeed;
					if (stateNode.has_child("PositionSet"))
						stateNode["PositionSet"] >> state.PositionSet;
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
					if (transitionNode.has_child("BlendDuration"))
						transitionNode["BlendDuration"] >> transition.BlendDuration;

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
							if (conditionNode.has_child("Type"))
								conditionNode["Type"] >> type;
							if (conditionNode.has_child("Operator"))
								conditionNode["Operator"] >> op;
							if (conditionNode.has_child("FloatValue"))
								conditionNode["FloatValue"] >> condition.FloatValue;
							if (conditionNode.has_child("BoolValue"))
								conditionNode["BoolValue"] >> condition.BoolValue;
							if (conditionNode.has_child("IntValue"))
								conditionNode["IntValue"] >> condition.IntValue;

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

	bool AnimationControllerSerializer::Serialize(const std::filesystem::path& filepath, const SharedPtr<AnimationController>& animationController)
	{
		if (!animationController)
			return false;

		auto outputPath = filepath;
		outputPath.replace_extension(".ebcontroller");

		ryml::Tree tree;
		auto root = tree.rootref();
		root |= ryml::MAP;

		root["Version"] << EBCONTROLLER_FILE_VERSION;

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

	SharedPtr<AnimationController> AnimationControllerSerializer::Deserialize(UUID uuid, const std::filesystem::path& filepath)
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
				if (parameterNode.has_child("Type"))
					parameterNode["Type"] >> type;
				if (parameterNode.has_child("FloatValue"))
					parameterNode["FloatValue"] >> parameter.FloatValue;
				if (parameterNode.has_child("BoolValue"))
					parameterNode["BoolValue"] >> parameter.BoolValue;
				if (parameterNode.has_child("IntValue"))
					parameterNode["IntValue"] >> parameter.IntValue;

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

				if (layerNode.has_child("Name"))
					layerNode["Name"] >> layer.Name;
				if (layerNode.has_child("Weight"))
					layerNode["Weight"] >> layer.Weight;
				if (layerNode.has_child("MaskHandle"))
					layerNode["MaskHandle"] >> maskHandle;

				layer.MaskHandle = UUID(maskHandle);
				DeserializeStateMachine(layerNode["StateMachine"], layer.StateMachine);
				layers.push_back(std::move(layer));
			}
		}

		return animationController;
	}

}
