#include "efpch.h"
#include "AnimationInspectorPanel.h"
#include "Viewers/AnimationViewportViewer.h"
#include "UI/PropertyGrid.h"
#include "UI/DragDropTypes.h"

#include <Ember/Asset/AssetManager.h>

#include <algorithm>
#include <vector>

namespace Ember {

	static const char* ConditionOperatorToString(AnimationConditionOperator op)
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

	static bool IsBoolLikeParameterType(AnimationParameterType type)
	{
		return type == AnimationParameterType::Bool || type == AnimationParameterType::Trigger;
	}

	static AnimationConditionOperator DefaultOperatorForType(AnimationParameterType type)
	{
		return IsBoolLikeParameterType(type)
			? AnimationConditionOperator::Equal
			: AnimationConditionOperator::GreaterThan;
	}

	static void BeginCleanTableHeaders()
	{
		// Give the header a subtle, darker tint that blends well with bgMid
		// This anchors the column names visually without making them look like 3D buttons
		ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0.11f, 0.11f, 0.11f, 1.0f));
	}

	static void EndCleanTableHeaders()
	{
		ImGui::PopStyleColor();
	}

	static bool DrawHighContrastCheckbox(const char* label, bool* v)
	{
		// Save current colors
		ImVec4 frameCol = ImGui::GetStyle().Colors[ImGuiCol_FrameBg];
		ImVec4 checkCol = ImGui::GetStyle().Colors[ImGuiCol_CheckMark];

		// Style: Make the checkbox frame visible and the checkmark pop
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.25f, 0.25f, 0.25f, 1.0f)); // Lighter gray box
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(1.0f, 0.5f, 0.0f, 1.0f)); // Ember Orange checkmark

		bool changed = ImGui::Checkbox(label, v);

		ImGui::PopStyleColor(3);
		return changed;
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

			// Render reverse transition if it exists
			auto stateMachine = animationViewport->GetAnimationStateMachine();
			auto toIdTransitionsIt = stateMachine->GetTransitions().find(selectedTransition->ToStateId);
			if (toIdTransitionsIt != stateMachine->GetTransitions().end())
			{
				for (auto& transition : toIdTransitionsIt->second)
				{
					if (transition.ToStateId == selectedTransition->FromStateId)
					{
						RenderAnimationTransition(&transition);
						break;
					}
				}
			}
		}

		ImGui::Separator();
		RenderAnimationParameters();
	}

	void AnimationInspectorPanel::RenderAnimationState(AnimationState* animState)
	{
		auto animationViewport = static_cast<AnimationViewportViewer*>(m_Context->ActiveViewportViewer);
		std::string nodeLabel = std::format("Animation State: {}###AnimStateNode_{}", animState->Name, animState->Id);
		if (UI::Nodes::BeginRemoveableExpandableNode(nodeLabel, [&]() {
			animationViewport->DeleteNode(animState->Id);
			}))
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

				auto browseFunc = [&]() { ImGui::OpenPopup("ChooseAnimPopup"); };
				auto clearFunc = animExists ? UI::UICallbackFunc([&]() { animState->AnimationHandle = Constants::InvalidUUID; }) : nullptr;

				if (UI::PropertyGrid::AssetReference("Animation", animName, payloadType, droppedPath, browseFunc, clearFunc))
				{
					if (auto animation = m_AssetManager.Load<Animation>(droppedPath))
						animState->AnimationHandle = animation->GetUUID();
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

		std::string fromStateName = getStateName(animTransition->FromStateId);
		std::string toStateName = getStateName(animTransition->ToStateId);
		std::string nodeLabel = std::format("Transition: {} -> {}###AnimTrans_{}", fromStateName, toStateName, animTransition->Id);

		// Helper lambda to check if this transition has a corresponding reverse transition (two way link)
		auto isTwoWayTransition = [&]() -> bool {
			if (animTransition->FromStateId != Constants::InvalidUUID && animTransition->ToStateId != Constants::InvalidUUID)
			{
				auto toIdTransitionsIt = stateMachine->GetTransitions().find(animTransition->ToStateId);
				if (toIdTransitionsIt != stateMachine->GetTransitions().end())
				{
					for (auto& transition : toIdTransitionsIt->second)
					{
						if (transition.ToStateId == animTransition->FromStateId)
						{
							return true;
						}
					}
				}
			}
			return false;
			}();

		auto setTwoWaySelectedTransition = [&](UUID toBeDeletedId)
			{
				// If the deleted transition is part of a two way link, select the other transition after deletion to keep the inspector populated
				auto toIdTransitionsIt = stateMachine->GetTransitions().find(animTransition->ToStateId);
				for (auto& transition : toIdTransitionsIt->second)
				{
					if (transition.ToStateId == animTransition->FromStateId && transition.Id != toBeDeletedId)
					{
						animationViewport->SetSelectedTransition(&transition);
						break;
					}
				}
			};

		if (UI::Nodes::BeginRemoveableExpandableNode(nodeLabel, [&]() {
			// Remove the transition from the state machine
			animationViewport->DeleteTransition(animTransition->Id);

			// If the deleted transition was part of a two way link, select the other transition after deletion to keep the inspector populated
			if (isTwoWayTransition)
				setTwoWaySelectedTransition(animTransition->Id);
			}))
		{
			RenderTransitionConditions(animTransition);
			UI::Nodes::EndExpandableNode();
		}
	}

	void AnimationInspectorPanel::RenderTransitionConditions(AnimationTransition* animTransition)
	{
		auto animationViewport = static_cast<AnimationViewportViewer*>(m_Context->ActiveViewportViewer);
		auto stateMachine = animationViewport->GetAnimationStateMachine();
		auto& parameters = stateMachine->GetParameters();

		// Header and Add Button aligned to the right
		ImGui::Spacing();
		ImGui::TextUnformatted("Conditions");
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.0f);
		if (ImGui::Button("+##AddCond", ImVec2(24, 24)))
		{
			if (!parameters.empty())
			{
				const std::string& defaultParamName = "New Parameter";
				const AnimationParameter & defaultParameter = AnimationParameter{ AnimationParameterType::Float, 0.0f, false, 0 };

				AnimationCondition condition;
				condition.ParameterName = defaultParamName;
				condition.Type = defaultParameter.Type;
				condition.Operator = DefaultOperatorForType(defaultParameter.Type);
				condition.FloatValue = defaultParameter.FloatValue;
				condition.BoolValue = defaultParameter.BoolValue;
				condition.IntValue = defaultParameter.IntValue;

				animTransition->Conditions.push_back(condition);
				animationViewport->MarkAnimationStateMachineDirty();
			}
		}

		if (animTransition->Conditions.empty())
		{
			ImGui::TextDisabled("  List is Empty");
			return;
		}

		ImGuiStyle& style = ImGui::GetStyle();

		// Accurately calculate the table height so it perfectly fits the rows without a scrollbar
		float headerHeight = ImGui::GetTextLineHeight() + (style.CellPadding.y * 2.0f);
		float rowHeight = ImGui::GetFrameHeight() + (style.CellPadding.y * 2.0f);
		float childHeight = headerHeight + (rowHeight * animTransition->Conditions.size()) + (style.WindowPadding.y * 2.0f) + 4.0f;

		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f); // Match EditorLayer theme

		ImGuiWindowFlags childFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		if (ImGui::BeginChild("##ConditionsList", ImVec2(0, childHeight), true, childFlags))
		{
			BeginCleanTableHeaders();

			if (ImGui::BeginTable("ConditionTable", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_BordersInnerH))
			{
				ImGui::TableSetupColumn("Parameter", ImGuiTableColumnFlags_WidthStretch, 0.60f);
				ImGui::TableSetupColumn("Condition", ImGuiTableColumnFlags_WidthStretch, 0.20f);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.15f);
				ImGui::TableSetupColumn("##Delete", ImGuiTableColumnFlags_WidthFixed, 24.0f);
				ImGui::TableHeadersRow();

				// Add padding
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Dummy(ImVec2(0, 4.0f));

				bool changed = false;
				for (size_t i = 0; i < animTransition->Conditions.size();)
				{
					AnimationCondition& condition = animTransition->Conditions[i];
					bool removeCondition = false;

					ImGui::PushID(static_cast<int>(i));
					ImGui::TableNextRow();

					// 1. Parameter Dropdown
					ImGui::TableSetColumnIndex(0);
					ImGui::SetNextItemWidth(-FLT_MIN);
					std::string preview = condition.ParameterName;
					if (!preview.empty() && !parameters.contains(preview))
						preview = std::format("Missing ({})", preview);

					if (ImGui::BeginCombo("##Param", preview.c_str()))
					{
						for (const auto& [paramName, paramData] : parameters)
						{
							if (ImGui::Selectable(paramName.c_str(), condition.ParameterName == paramName))
							{
								condition.ParameterName = paramName;
								condition.Type = paramData.Type;
								condition.Operator = DefaultOperatorForType(paramData.Type);
								changed = true;
							}
						}
						ImGui::EndCombo();
					}

					// 2. Operator Dropdown
					ImGui::TableSetColumnIndex(1);
					ImGui::SetNextItemWidth(-FLT_MIN);
					const bool isBoolLike = IsBoolLikeParameterType(condition.Type);
					if (ImGui::BeginCombo("##Op", ConditionOperatorToString(condition.Operator)))
					{
						const std::vector<AnimationConditionOperator> ops = isBoolLike
							? std::vector{ AnimationConditionOperator::Equal, AnimationConditionOperator::NotEqual }
						: std::vector{ AnimationConditionOperator::GreaterThan, AnimationConditionOperator::LessThan, AnimationConditionOperator::Equal, AnimationConditionOperator::NotEqual };

						for (auto op : ops)
						{
							if (ImGui::Selectable(ConditionOperatorToString(op), condition.Operator == op))
							{
								condition.Operator = op;
								changed = true;
							}
						}
						ImGui::EndCombo();
					}

					// 3. Value Input
					ImGui::TableSetColumnIndex(2);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (condition.Type == AnimationParameterType::Float)
						changed |= ImGui::DragFloat("##ValF", &condition.FloatValue, 0.1f);
					else if (condition.Type == AnimationParameterType::Int)
						changed |= ImGui::DragInt("##ValI", &condition.IntValue, 1.0f);
					else if (isBoolLike)
						changed |= DrawHighContrastCheckbox("##ValB", &condition.BoolValue);

					// 4. Delete Button
					ImGui::TableSetColumnIndex(3);
					if (ImGui::Button("-", ImVec2(24, 0)))
						removeCondition = true;

					ImGui::PopID();

					if (removeCondition)
					{
						animTransition->Conditions.erase(animTransition->Conditions.begin() + i);
						changed = true;
					}
					else
					{
						++i;
					}
				}
				ImGui::EndTable();

				if (changed)
					animationViewport->MarkAnimationStateMachineDirty();
			}
			EndCleanTableHeaders();
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	void AnimationInspectorPanel::RenderAnimationParameters()
	{
		ImGui::Spacing();
		ImGui::TextUnformatted("Parameters");
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.0f);
		if (ImGui::Button("+##AddParam", ImVec2(24, 24)))
		{
			m_NewParameterName.clear();
			m_NewParameterType = AnimationParameterType::Float;
			ImGui::OpenPopup("AddAnimationParameterPopup");
		}

		RenderAddParameterPopup();
		RenderAnimationParametersTable();
	}

	void AnimationInspectorPanel::RenderAddParameterPopup()
	{
		auto animationViewport = static_cast<AnimationViewportViewer*>(m_Context->ActiveViewportViewer);
		auto stateMachine = animationViewport->GetAnimationStateMachine();

		if (ImGui::BeginPopup("AddAnimationParameterPopup"))
		{
			if (UI::PropertyGrid::Begin("NewParamProps"))
			{
				std::string uniqueName = m_NewParameterName.empty() ? "New Parameter" : m_NewParameterName;
				if (UI::PropertyGrid::InputText("Name", uniqueName))
					m_NewParameterName = uniqueName;

				if (UI::PropertyGrid::BeginComboBox("Type", ParameterTypeToString(m_NewParameterType)))
				{
					for (auto type : { AnimationParameterType::Float, AnimationParameterType::Bool, AnimationParameterType::Int, AnimationParameterType::Trigger })
					{
						if (ImGui::Selectable(ParameterTypeToString(type), m_NewParameterType == type))
							m_NewParameterType = type;
					}
					UI::PropertyGrid::EndComboBox();
				}

				UI::PropertyGrid::End();
			}

			const std::string parameterName = m_NewParameterName.empty() ? "New Parameter" : m_NewParameterName;
			bool canCreate = !parameterName.empty() && !stateMachine->GetParameters().contains(parameterName);
			if (!canCreate)
				ImGui::BeginDisabled();

			if (ImGui::Button("Create", ImVec2(120.0f, 0.0f)))
			{
				stateMachine->AddParameter(parameterName, m_NewParameterType);
				m_NewParameterName.clear();
				m_NewParameterType = AnimationParameterType::Float;
				animationViewport->MarkAnimationStateMachineDirty();
				ImGui::CloseCurrentPopup();
			}

			if (!canCreate)
				ImGui::EndDisabled();

			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}

	void AnimationInspectorPanel::RenderAnimationParametersTable()
	{
		auto animationViewport = static_cast<AnimationViewportViewer*>(m_Context->ActiveViewportViewer);
		auto stateMachine = animationViewport->GetAnimationStateMachine();
		auto& parameters = stateMachine->GetParameters();

		if (parameters.empty())
		{
			ImGui::TextDisabled("  No Parameters");
			return;
		}

		ImGuiStyle& style = ImGui::GetStyle();
		float headerHeight = ImGui::GetTextLineHeight() + (style.CellPadding.y * 2.0f);
		float rowHeight = ImGui::GetFrameHeight() + (style.CellPadding.y * 2.0f);
		float childHeight = headerHeight + (rowHeight * parameters.size()) + (style.WindowPadding.y * 2.0f) + 4.0f;

		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);

		ImGuiWindowFlags childFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		if (ImGui::BeginChild("##ParamsList", ImVec2(0, childHeight), true, childFlags))
		{
			BeginCleanTableHeaders();

			if (ImGui::BeginTable("ParamsTable", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_BordersInnerH))
			{
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.50f);
				ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 0.25f);
				ImGui::TableSetupColumn("Default", ImGuiTableColumnFlags_WidthStretch, 0.15f);
				ImGui::TableSetupColumn("##Del", ImGuiTableColumnFlags_WidthFixed, 24.0f);
				ImGui::TableHeadersRow();

				// Add padding
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Dummy(ImVec2(0, 4.0f));

				std::vector<std::string> sortedNames;
				for (const auto& [name, _] : parameters) sortedNames.push_back(name);
				std::sort(sortedNames.begin(), sortedNames.end());

				bool changed = false;
				for (const auto& name : sortedNames)
				{
					AnimationParameter param = parameters.at(name);
					bool rowChanged = false;
					bool renamed = false;

					ImGui::PushID(name.c_str());
					ImGui::TableNextRow();

					// 1. Name / Rename
					ImGui::TableSetColumnIndex(0);
					if (m_RenamingParameterName == name)
					{
						char renameBuf[256];
						strncpy_s(renameBuf, sizeof(renameBuf), m_RenameBuffer.c_str(), _TRUNCATE);
						ImGui::SetNextItemWidth(-FLT_MIN);
						if (ImGui::InputText("##Ren", renameBuf, sizeof(renameBuf), ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::IsItemDeactivatedAfterEdit())
						{
							std::string newName = renameBuf;
							if (!newName.empty() && newName != name && !parameters.contains(newName))
							{
								parameters.erase(name);
								parameters[newName] = param;
								changed = true;
								renamed = true;
							}
							m_RenamingParameterName.clear();
						}
						else
						{
							m_RenameBuffer = renameBuf;
						}
					}
					else
					{
						ImGui::TextUnformatted(name.c_str());
						if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
						{
							m_RenamingParameterName = name;
							m_RenameBuffer = name;
						}
					}

					// 2. Type Dropdown
					ImGui::TableSetColumnIndex(1);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::BeginCombo("##Type", ParameterTypeToString(param.Type)))
					{
						for (auto type : { AnimationParameterType::Float, AnimationParameterType::Bool, AnimationParameterType::Int, AnimationParameterType::Trigger })
						{
							if (ImGui::Selectable(ParameterTypeToString(type), param.Type == type))
							{
								param.Type = type;
								param.FloatValue = 0.0f; param.IntValue = 0; param.BoolValue = false;
								parameters[name] = param;
								changed = true;
							}
						}
						ImGui::EndCombo();
					}

					// 3. Value Input
					ImGui::TableSetColumnIndex(2);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (param.Type == AnimationParameterType::Float)
						rowChanged |= ImGui::DragFloat("##DefF", &param.FloatValue, 0.1f);
					else if (param.Type == AnimationParameterType::Int)
						rowChanged |= ImGui::DragInt("##DefI", &param.IntValue, 1.0f);
					else if (param.Type == AnimationParameterType::Bool || param.Type == AnimationParameterType::Trigger)
						rowChanged |= DrawHighContrastCheckbox("##DefB", &param.BoolValue);

					if (rowChanged && !renamed && parameters.contains(name))
					{
						parameters[name] = param;
						changed = true;
					}

					// 4. Delete
					ImGui::TableSetColumnIndex(3);
					if (ImGui::Button("-", ImVec2(24, 0)))
					{
						stateMachine->RemoveParameter(name);
						changed = true;
						ImGui::PopID();
						break;
					}

					ImGui::PopID();
				}
				ImGui::EndTable();

				if (changed)
					animationViewport->MarkAnimationStateMachineDirty();
			}
			EndCleanTableHeaders();
		}
		ImGui::EndChild();
		ImGui::PopStyleVar();
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
}