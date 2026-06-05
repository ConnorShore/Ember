#include "efpch.h"
#include "AnimationViewportViewer.h"
#include "EditorLayer.h"
#include <imgui.h>
#include <format>

namespace Ember {

	static uint32_t GetNextId()
	{
		static uint32_t id = 1;
		return id++;
	}

	AnimationViewportViewer::AnimationViewportViewer(SharedPtr<Scene> scene, SharedPtr<AnimationStateMachine> animationStateMachine, const std::string& filePath, const std::string& title)
		: EditorViewportViewer(Type::Animation, scene, filePath, title), m_AnimationStateMachine(animationStateMachine)
	{
		ne::Config config;
		config.SettingsFile = "AnimationNodeEditor.json";
		m_NodeEditorContext = ne::CreateEditor(&config);
	}

	AnimationViewportViewer::~AnimationViewportViewer()
	{
		ne::DestroyEditor(m_NodeEditorContext);
	}

	void AnimationViewportViewer::OnImGuiRender(EditorLayer* editor)
	{
		editor->SetViewportHovered(ImGui::IsWindowHovered());
		editor->SetViewportFocused(ImGui::IsWindowFocused());

		ne::SetCurrentEditor(m_NodeEditorContext);
		ne::Begin(std::format("##AnimationGraph_{}", m_AnimationStateMachine->GetName()).c_str());

		DrawAllNodes();

		ne::End();
		ne::SetCurrentEditor(nullptr);
	}

	void AnimationViewportViewer::RebuildGraph()
	{
		m_Nodes.clear();
		m_Links.clear();

		// Build all Nodes
		for (auto& [stateId, state] : m_AnimationStateMachine->GetStates())
		{
			// Emplace first so the memory address is stable
			m_Nodes[stateId] = Node(GetNextId(), state.Name.c_str(), ImColor(64, 128, 255));
			Node& node = m_Nodes[stateId];

			node.Inputs.emplace_back(GetNextId(), "", PinType::Flow);
			node.Outputs.emplace_back(GetNextId(), "", PinType::Flow);

			// Safely assign the parent pointer
			node.Inputs[0].Node = &node;
			node.Outputs[0].Node = &node;
		}

		// Build all Links (Transitions)
		for (auto& [stateId, transitions] : m_AnimationStateMachine->GetTransitions())
		{
			if (m_Nodes.find(stateId) == m_Nodes.end()) continue;
			Node& startNode = m_Nodes[stateId];

			for (auto& transition : transitions)
			{
				if (m_Nodes.find(transition.ToStateId) == m_Nodes.end()) continue;
				Node& endNode = m_Nodes[transition.ToStateId];

				uint64_t linkId = GetNextId();
				m_Links[linkId] = Link(linkId, startNode.Outputs[0].ID, endNode.Inputs[0].ID);
			}
		}

		m_GraphNeedsRebuild = false;
	}

	void AnimationViewportViewer::DrawAllNodes()
	{
		// Only build the data structures once!
		if (m_GraphNeedsRebuild)
		{
			RebuildGraph();
		}

		// TECHNIQUE 1: Force straight wires (Override bezier curve strength to 0)
		ne::PushStyleVar(ne::StyleVar_LinkStrength, 0.0f);

		// 1. Draw all Nodes
		for (auto& [name, node] : m_Nodes)
		{
			ne::BeginNode(node.ID);

			// Draw the State Name
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
			ImGui::TextUnformatted(node.Name.c_str());
			ImGui::PopStyleColor();

			// TECHNIQUE 2: The "Center Pin" Illusion
			ImVec2 nodeSize = ImGui::GetItemRectSize();
			ImVec2 nodePos = ImGui::GetItemRectMin();
			ImVec2 centerPos = ImVec2(nodePos.x + (nodeSize.x * 0.5f), nodePos.y + (nodeSize.y * 0.5f));

			// Draw Input Pin (Center, Invisible anchor point)
			ImGui::SetCursorScreenPos(centerPos);
			ne::BeginPin(node.Inputs[0].ID, ne::PinKind::Input);
			ImGui::Dummy(ImVec2(1, 1));
			ne::EndPin();

			// Draw Output Pin (Center, Invisible anchor point)
			ImGui::SetCursorScreenPos(centerPos);
			ne::BeginPin(node.Outputs[0].ID, ne::PinKind::Output);
			ImGui::Dummy(ImVec2(1, 1));
			ne::EndPin();

			ne::EndNode();
		}

		// Draw all Links (Transitions)
		for (auto& [id, link] : m_Links)
		{
			ne::Link(link.ID, link.StartPinID, link.EndPinID, link.Color, 2.0f);

			// Animate a pulsing flow down the wire to visually indicate the direction 
			// of the transition, instantly solving the bidirectional ambiguity problem!
			ne::Flow(link.ID);
		}

		ne::PopStyleVar();
	}
}