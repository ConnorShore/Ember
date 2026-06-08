
#include "efpch.h"
#include "AnimationInspectorPanel.h"
#include "Viewers/AnimationViewportViewer.h"
#include "UI/PropertyGrid.h"
#include "UI/DragDropTypes.h"

#include <Ember/Asset/AssetManager.h>

#include <algorithm>
#include <vector>

namespace Ember {

	namespace
	{
		const char* ConditionOperatorToString(AnimationConditionOperator op)
		{
			switch (op)
			{
			case AnimationConditionOperator::GreaterThan: return ">";
			case AnimationConditionOperator::LessThan: return "<";
			case AnimationConditionOperator::Equal: return "==";
			case AnimationConditionOperator::NotEqual: return "!=";
			default: return "?";
			}
		}

		bool IsBoolLikeParameterType(AnimationParameterType type)
		{
			return type == AnimationParameterType::Bool || type == AnimationParameterType::Trigger;
		}

		AnimationConditionOperator DefaultOperatorForType(AnimationParameterType type)
		{
			return IsBoolLikeParameterType(type)
				? AnimationConditionOperator::Equal
				: AnimationConditionOperator::GreaterThan;
		}
	}

	AnimationInspectorPanel::AnimationInspectorPanel(EditorContext* context)
		: InspectorPanelContent(context), m_AssetManager(Application::Instance().GetAssetManager())
	{
	}

	AnimationInspectorPanel::~AnimationInspectorPanel()
	{
	}

	void AnimationInspectorPanel::OnImGuiRender()
	{
		auto animationViewport = static_cast<AnimationViewportViewer*>(m_Context->ActiveViewportViewer);
		
		auto selectedState = animationViewport->GetSelectedState();
		auto selectedTransition = animationViewport->GetSelectedTransition();

		if (selectedState)
			RenderAnimationState(selectedState);

		if (selectedTransition) 
		{
			RenderAnimationTransition(selectedTransition);

			// If there is a transition going the other way as well, render that one too
			auto stateMachine = animationViewport->GetAnimationStateMachine();
			auto toId = selectedTransition->ToStateId;
			auto toIdTransitionsIt = stateMachine->GetTransitions().find(toId);
			for (auto& transition : toIdTransitionsIt->second)
			{
				if (transition.ToStateId == selectedTransition->FromStateId)
				{
					RenderAnimationTransition(&transition);
					break;
				}
			}
		}

		ImGui::Separator();
		RenderAnimationParameters();
	}

	void AnimationInspectorPanel::RenderAnimationState(AnimationState* animState)
	{
		std::string nodeLabel = std::format("Animation State: {}###AnimStateNode_{}", animState->Name, animState->Id);
		if (UI::Nodes::BeginExpandableNode(nodeLabel))
		{
			if (UI::PropertyGrid::Begin("AnimStateProps"))
			{
				if (UI::PropertyGrid::InputText("Name", animState->Name))
				{
					auto animationViewport = static_cast<AnimationViewportViewer*>(m_Context->ActiveViewportViewer);
					animationViewport->RenameNode(animState->Id, animState->Name);
				}

				bool animExists = animState->AnimationHandle != Constants::InvalidUUID;
				std::string animName = "None (Animation)";
				if (animExists)
				{
					auto animAsset = m_AssetManager.GetAsset<Animation>(animState->AnimationHandle);
					if (animAsset)
						animName = std::filesystem::path(animAsset->GetFilePath()).filename().string();
				}

				std::string payloadType = DragDropUtils::DragDropPayloadTypeToString(DragDropPayloadType::AssetAnimation);
				std::string droppedPath;

				auto browseFunc = [&]() {
					ImGui::OpenPopup("ChooseAnimPopup");
					};

				auto clearFunc = animExists ? UI::UICallbackFunc([&]() {
					animState->AnimationHandle = Constants::InvalidUUID;
					}) : nullptr;

				if (UI::PropertyGrid::AssetReference("Animation", animName, payloadType, droppedPath, browseFunc, clearFunc))
				{
					auto animation = m_AssetManager.Load<Animation>(droppedPath);
					if (animation)
					{
						animState->AnimationHandle = animation->GetUUID();
					}
				}

				UI::PropertyGrid::Checkbox("Looping", animState->Looping);
				UI::PropertyGrid::Float("Base Playback Speed", animState->BasePlaybackSpeed);

				UI::PropertyGrid::End();
			}

			UI::Nodes::EndExpandableNode();
		}
	}

	void AnimationInspectorPanel::RenderAnimationTransition(AnimationTransition* animTransition)
	{
		auto animationViewport = static_cast<AnimationViewportViewer*>(m_Context->ActiveViewportViewer);
		auto stateMachine = animationViewport->GetAnimationStateMachine();

		auto getStateName = [&](UUID stateId) -> std::string {
			if (stateId != Constants::InvalidUUID)
			{
				auto stateIt = stateMachine->GetStates().find(stateId);
				if (stateIt != stateMachine->GetStates().end())
					return stateIt->second.Name;
			}
			return "None";
			};

		std::string toStateName = getStateName(animTransition->ToStateId);
		std::string fromStateName = getStateName(animTransition->FromStateId);

		std::string nodeLabel = std::format("Animation Transition: {} --> {}###AnimTransition_{}", fromStateName, toStateName, animTransition->Id);
		if (UI::Nodes::BeginExpandableNode(nodeLabel))
		{
			if (UI::PropertyGrid::Begin("AnimTransitionProps"))
			{
				UI::PropertyGrid::LabelWithValue("From State", fromStateName);
				UI::PropertyGrid::LabelWithValue("To State", toStateName);
				UI::PropertyGrid::End();
			}

			// Render conditions
			auto& parameters = stateMachine->GetParameters();
			std::vector<std::string> parameterNames;
			parameterNames.reserve(parameters.size());
			for (const auto& [name, parameter] : parameters)
				parameterNames.push_back(name);
			std::sort(parameterNames.begin(), parameterNames.end());

			bool changed = false;

			ImGui::SeparatorText("Conditions");
			if (ImGui::Button("Add Condition"))
			{
				if (!parameterNames.empty())
				{
					const std::string& defaultParamName = parameterNames.front();
					const AnimationParameter& defaultParameter = parameters.at(defaultParamName);

					AnimationCondition condition;
					condition.ParameterName = defaultParamName;
					condition.Type = defaultParameter.Type;
					condition.Operator = DefaultOperatorForType(defaultParameter.Type);
					condition.FloatValue = defaultParameter.FloatValue;
					condition.BoolValue = defaultParameter.BoolValue;
					condition.IntValue = defaultParameter.IntValue;

					animTransition->Conditions.push_back(condition);
					changed = true;
				}
			}

			if (parameterNames.empty())
				ImGui::TextDisabled("Create animation parameters to add transition conditions.");

			if (!animTransition->Conditions.empty())
			{
				if (ImGui::BeginTable("TransitionConditionTable", 4, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
				{
					ImGui::TableSetupColumn("Parameter", ImGuiTableColumnFlags_WidthStretch, 0.42f);
					ImGui::TableSetupColumn("Operator", ImGuiTableColumnFlags_WidthStretch, 0.16f);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.34f);
					ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 32.0f);
					ImGui::TableHeadersRow();

					for (size_t i = 0; i < animTransition->Conditions.size();)
					{
						AnimationCondition& condition = animTransition->Conditions[i];
						bool removeCondition = false;

						ImGui::PushID(static_cast<int>(i));
						ImGui::TableNextRow();

						ImGui::TableSetColumnIndex(0);
						ImGui::SetNextItemWidth(-FLT_MIN);

						std::string parameterPreview = condition.ParameterName;
						if (!condition.ParameterName.empty() && !parameters.contains(condition.ParameterName))
							parameterPreview = std::format("Missing ({})", condition.ParameterName);

						if (ImGui::BeginCombo("##ConditionParameter", parameterPreview.c_str()))
						{
							for (const std::string& parameterName : parameterNames)
							{
								bool isSelected = condition.ParameterName == parameterName;
								if (ImGui::Selectable(parameterName.c_str(), isSelected))
								{
									const AnimationParameter& parameter = parameters.at(parameterName);
									const AnimationParameterType previousType = condition.Type;
									condition.ParameterName = parameterName;
									condition.Type = parameter.Type;

									if (IsBoolLikeParameterType(condition.Type) && !IsBoolLikeParameterType(previousType))
										condition.Operator = AnimationConditionOperator::Equal;
									else if (IsBoolLikeParameterType(condition.Type) &&
										!(condition.Operator == AnimationConditionOperator::Equal || condition.Operator == AnimationConditionOperator::NotEqual))
										condition.Operator = AnimationConditionOperator::Equal;

									changed = true;
								}
								if (isSelected)
									ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}

						ImGui::TableSetColumnIndex(1);
						ImGui::SetNextItemWidth(-FLT_MIN);

						const bool boolLike = IsBoolLikeParameterType(condition.Type);
						if (ImGui::BeginCombo("##ConditionOperator", ConditionOperatorToString(condition.Operator)))
						{
							const std::initializer_list<AnimationConditionOperator> boolOps = {
								AnimationConditionOperator::Equal,
								AnimationConditionOperator::NotEqual
							};
							const std::initializer_list<AnimationConditionOperator> numericOps = {
								AnimationConditionOperator::GreaterThan,
								AnimationConditionOperator::LessThan,
								AnimationConditionOperator::Equal,
								AnimationConditionOperator::NotEqual
							};

							const auto& ops = boolLike ? boolOps : numericOps;
							for (AnimationConditionOperator op : ops)
							{
								bool isSelected = condition.Operator == op;
								if (ImGui::Selectable(ConditionOperatorToString(op), isSelected))
								{
									condition.Operator = op;
									changed = true;
								}
								if (isSelected)
									ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}

						ImGui::TableSetColumnIndex(2);
						ImGui::SetNextItemWidth(-FLT_MIN);
						switch (condition.Type)
						{
						case AnimationParameterType::Float:
							if (ImGui::DragFloat("##ConditionFloat", &condition.FloatValue, 0.1f))
								changed = true;
							break;
						case AnimationParameterType::Int:
							if (ImGui::DragInt("##ConditionInt", &condition.IntValue, 1.0f))
								changed = true;
							break;
						case AnimationParameterType::Bool:
						case AnimationParameterType::Trigger:
							if (ImGui::Checkbox("##ConditionBool", &condition.BoolValue))
								changed = true;
							break;
						default:
							break;
						}

						ImGui::TableSetColumnIndex(3);
						if (ImGui::Button("X", ImVec2(-FLT_MIN, 0.0f)))
							removeCondition = true;

						ImGui::PopID();

						if (removeCondition)
						{
							animTransition->Conditions.erase(animTransition->Conditions.begin() + static_cast<int64_t>(i));
							changed = true;
							continue;
						}

						++i;
					}

					ImGui::EndTable();
				}
			}
			else
			{
				ImGui::TextDisabled("No transition conditions.");
			}

			if (changed)
				animationViewport->MarkAnimationStateMachineDirty();


			UI::Nodes::EndExpandableNode();
		}
	}

	const char* AnimationInspectorPanel::ParameterTypeToString(AnimationParameterType type) const
	{
		switch (type)
		{
		case AnimationParameterType::Float: return "Float";
		case AnimationParameterType::Bool: return "Bool";
		case AnimationParameterType::Int: return "Int";
		case AnimationParameterType::Trigger: return "Trigger";
		default: return "Unknown";
		}
	}

	void AnimationInspectorPanel::RenderAnimationParameters()
	{
		auto animationViewport = static_cast<AnimationViewportViewer*>(m_Context->ActiveViewportViewer);
		auto stateMachine = animationViewport->GetAnimationStateMachine();
		auto& parameters = stateMachine->GetParameters();

		bool changed = false;

		ImGui::TextUnformatted("Animation Parameters");
		ImGui::SameLine();
		if (ImGui::Button("Add Parameter"))
		{
			m_NewParameterName.clear();
			m_NewParameterType = AnimationParameterType::Float;
			ImGui::OpenPopup("AddAnimationParameterPopup");
		}

		if (ImGui::BeginPopup("AddAnimationParameterPopup"))
		{
			char nameBuffer[256];
			strncpy_s(nameBuffer, sizeof(nameBuffer), m_NewParameterName.c_str(), _TRUNCATE);
			if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
				m_NewParameterName = nameBuffer;

			if (ImGui::BeginCombo("Type", ParameterTypeToString(m_NewParameterType)))
			{
				for (AnimationParameterType type : { AnimationParameterType::Float, AnimationParameterType::Bool, AnimationParameterType::Int, AnimationParameterType::Trigger })
				{
					bool isSelected = m_NewParameterType == type;
					if (ImGui::Selectable(ParameterTypeToString(type), isSelected))
						m_NewParameterType = type;
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			bool canCreate = !m_NewParameterName.empty() && !parameters.contains(m_NewParameterName);
			if (!canCreate)
				ImGui::BeginDisabled();
			if (ImGui::Button("Create", ImVec2(120.0f, 0.0f)))
			{
				stateMachine->AddParameter(m_NewParameterName, m_NewParameterType);
				m_NewParameterName.clear();
				m_NewParameterType = AnimationParameterType::Float;
				changed = true;
				ImGui::CloseCurrentPopup();
			}
			if (!canCreate)
				ImGui::EndDisabled();

			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}

		ImGui::Spacing();

		if (ImGui::BeginTable("AnimationParameterTable", 4, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.44f);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 0.2f);
			ImGui::TableSetupColumn("Default", ImGuiTableColumnFlags_WidthStretch, 0.28f);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 32.0f);
			ImGui::TableHeadersRow();

			std::vector<std::string> parameterNames;
			parameterNames.reserve(parameters.size());
			for (const auto& [name, parameter] : parameters)
				parameterNames.push_back(name);
			std::sort(parameterNames.begin(), parameterNames.end());

			for (const auto& parameterName : parameterNames)
			{
				if (!parameters.contains(parameterName))
					continue;

				AnimationParameter parameter = parameters.at(parameterName);

				ImGui::PushID(parameterName.c_str());
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				if (m_RenamingParameterName == parameterName)
				{
					char renameBuffer[256];
					strncpy_s(renameBuffer, sizeof(renameBuffer), m_RenameBuffer.c_str(), _TRUNCATE);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::InputText("##Rename", renameBuffer, sizeof(renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
					{
						std::string newName = renameBuffer;
						if (!newName.empty() && newName != parameterName && !parameters.contains(newName))
						{
							parameters.erase(parameterName);
							parameters[newName] = parameter;
							changed = true;
						}
						m_RenamingParameterName.clear();
						m_RenameBuffer.clear();
					}
					else
					{
						m_RenameBuffer = renameBuffer;
					}

					if (ImGui::IsItemDeactivatedAfterEdit())
					{
						std::string newName = m_RenameBuffer;
						if (!newName.empty() && newName != parameterName && !parameters.contains(newName))
						{
							parameters.erase(parameterName);
							parameters[newName] = parameter;
							changed = true;
						}
						m_RenamingParameterName.clear();
						m_RenameBuffer.clear();
					}
				}
				else
				{
					ImGui::TextUnformatted(parameterName.c_str());
					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					{
						m_RenamingParameterName = parameterName;
						m_RenameBuffer = parameterName;
					}
				}

				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::BeginCombo("##Type", ParameterTypeToString(parameter.Type)))
				{
					for (AnimationParameterType type : { AnimationParameterType::Float, AnimationParameterType::Bool, AnimationParameterType::Int, AnimationParameterType::Trigger })
					{
						bool isSelected = parameter.Type == type;
						if (ImGui::Selectable(ParameterTypeToString(type), isSelected))
						{
							parameter.Type = type;
							parameter.FloatValue = 0.0f;
							parameter.BoolValue = false;
							parameter.IntValue = 0;
							parameters[parameterName] = parameter;
							changed = true;
						}
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				ImGui::TableSetColumnIndex(2);
				ImGui::SetNextItemWidth(-FLT_MIN);
				switch (parameter.Type)
				{
				case AnimationParameterType::Float:
				{
					float value = parameter.FloatValue;
					if (ImGui::DragFloat("##DefaultFloat", &value, 0.1f))
					{
						parameter.FloatValue = value;
						parameters[parameterName] = parameter;
						changed = true;
					}
					break;
				}
				case AnimationParameterType::Bool:
				case AnimationParameterType::Trigger:
				{
					bool value = parameter.BoolValue;
					if (ImGui::Checkbox("##DefaultBool", &value))
					{
						parameter.BoolValue = value;
						parameters[parameterName] = parameter;
						changed = true;
					}
					break;
				}
				case AnimationParameterType::Int:
				{
					int value = parameter.IntValue;
					if (ImGui::DragInt("##DefaultInt", &value, 1.0f))
					{
						parameter.IntValue = value;
						parameters[parameterName] = parameter;
						changed = true;
					}
					break;
				}
				default:
					break;
				}

				ImGui::TableSetColumnIndex(3);
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::Button("X", ImVec2(-FLT_MIN, 0.0f)))
				{
					stateMachine->RemoveParameter(parameterName);
					if (m_RenamingParameterName == parameterName)
					{
						m_RenamingParameterName.clear();
						m_RenameBuffer.clear();
					}
					changed = true;
					ImGui::PopID();
					break;
				}

				ImGui::PopID();
			}

			ImGui::EndTable();
		}

		if (changed)
			animationViewport->MarkAnimationStateMachineDirty();
	}

}
