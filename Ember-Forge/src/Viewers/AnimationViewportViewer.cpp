#include "efpch.h"
#include "AnimationViewportViewer.h"
#include "EditorLayer.h"

#include <Ember/Core/ProjectManager.h>
#include <Ember/Animation/AnimationStateMachineSerializer.h>
#include <Ember/Event/UIEvent.h>
#include <Ember/Input/Input.h>
#include <imgui/imgui.h>

#include <string>
#include <format>

namespace Ember {

	// Ids that are reserved for the Entry and Exit nodes and their pins and links.
	// Ids associated with actual states/transitions will use their UUIDs
	static uint32_t GetNextId()
	{
		static uint32_t id = 1;
		return id++;
	}

	AnimationViewportViewer::AnimationViewportViewer(SharedPtr<Scene> scene, SharedPtr<AnimationStateMachine> animationStateMachine, const std::string& filePath, const std::string& title)
		: EditorViewportViewer(Type::Animation, scene, filePath, title), m_AnimationStateMachine(animationStateMachine)
	{
		ne::Config config;
		std::string path = (ProjectManager::GetActive()->GetProjectDirectory() / "AnimationNodeEditor.json").string();
		//config.SettingsFile = path.c_str();
		config.SettingsFile = nullptr; // Disable built-in persistence for now since we want to handle it ourselves
		m_NodeEditorContext = ne::CreateEditor(&config);

		// Draw Default Nodes
		m_EntryNode = Node(ENTRY_NODE_ID, "Start", ImColor(64, 255, 128));
		m_EntryNode.Outputs.emplace_back(ENTRY_PIN_ID, "", PinType::Flow);
		m_EntryNode.Outputs[0].Node = &m_EntryNode;

		m_ExitNode = Node(EXIT_NODE_ID, "End", ImColor(255, 64, 128));
		m_ExitNode.Inputs.emplace_back(EXIT_PIN_ID, "", PinType::Flow);
		m_ExitNode.Inputs[0].Node = &m_ExitNode;
	}

	AnimationViewportViewer::~AnimationViewportViewer()
	{
		ne::DestroyEditor(m_NodeEditorContext);
	}

	void AnimationViewportViewer::OnOpen(EditorLayer* editor)
	{
		// Mark all states to set position
		m_AnimationStateMachine->EntryNodePositionSet = false;
		m_AnimationStateMachine->ExitNodePositionSet = false;

		for (auto& [stateId, state] : m_AnimationStateMachine->GetStates())
			state.PositionSet = false;
	}

	void AnimationViewportViewer::OnUpdate(TimeStep delta, EditorLayer* editor)
	{
		if (m_IsDirty)
		{
			m_SaveCooldown -= delta;

			if (m_SaveCooldown <= 0.0f)
			{
				SaveAnimationStateMachine(editor);
			}
		}
	}

	void AnimationViewportViewer::OnImGuiRender(EditorLayer* editor)
	{
		editor->SetViewportHovered(ImGui::IsWindowHovered());
		editor->SetViewportFocused(ImGui::IsWindowFocused());

		ne::SetCurrentEditor(m_NodeEditorContext);
		ne::Begin(std::format("##AnimationGraph_{}", m_AnimationStateMachine->GetName()).c_str());

		HandleHotkeys();
		DrawAllNodes();
		HandleInteractiveTransition();
		RenderContextMenus();

		ne::End();

		if (m_InteractiveTransitionOrigin.Get() != Constants::InvalidUUID)
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddLine(m_InteractiveTransitionScreenStart, ImGui::GetMousePos(), IM_COL32(240, 120, 30, 255), 3.0f);
		}

		// See if any nodes have been selected
		CheckNodeSelected();

		ne::SetCurrentEditor(nullptr);
	}

	void AnimationViewportViewer::HandleHotkeys()
	{
		// Check for delete key to delete selected node or link
		if (Input::IsKeyPressed(KeyCode::Delete))
		{
			if (m_SelectedState)
				DeleteNode(m_SelectedState->Id);
			//else if (m_SelectedTransition)
			//	DeleteTransition(m_SelectedTransition->Id);
		}
	}

	void AnimationViewportViewer::SaveAnimationStateMachine(EditorLayer* editor)
	{
		AnimationStateMachineSerializer::Serialize(m_AnimationStateMachine->GetFilePath(), m_AnimationStateMachine);
		m_IsDirty = false;

		// Auto-save notification
		auto evt = UINotificationEvent("Auto-Saved Animation State Machine", UINotificationEvent::Severity::Info);
		editor->GetContext().EventCallback(evt);
	}

	void AnimationViewportViewer::RebuildGraph()
	{
		m_Nodes.clear();
		m_Links.clear();

		// Build all Nodes
		for (auto& [stateId, state] : m_AnimationStateMachine->GetStates())
		{
			// Emplace first so the memory address is stable
			m_Nodes[stateId] = Node(stateId, state.Name.c_str(), ImColor(64, 128, 255));
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
				if (m_Nodes.find(transition.ToStateId) == m_Nodes.end())
					continue;

				Node& endNode = m_Nodes[transition.ToStateId];

				//uint64_t linkId = GetNextId();
				uint64_t linkId = transition.Id;
				m_Links[linkId] = Link(linkId, startNode.Outputs[0].ID, endNode.Inputs[0].ID);
			}
		}

		// Connect Entry Node to Default State
		m_Links[ENTRY_LINK_ID] = Link(ENTRY_LINK_ID, 
			m_EntryNode.Outputs[0].ID, 
			m_Nodes[m_AnimationStateMachine->GetDefaultState()].Inputs[0].ID, 
			ImColor(30, 190, 30)
		);

		// Connect all States with no outgoing transitions to the Exit Node
		for (auto& [stateId, node] : m_Nodes)
		{
			if (m_AnimationStateMachine->GetTransitions().find(stateId) == m_AnimationStateMachine->GetTransitions().end())
			{
				uint64_t linkId = GetNextId();	// This is just a visual link so we don't need to use the actual transition ID since it doesn't exist in the data
				m_Links[linkId] = Link(linkId, node.Outputs[0].ID, m_ExitNode.Inputs[0].ID, ImColor(190, 30, 30));
			}
		}

		m_GraphNeedsRebuild = false;
	}

	void AnimationViewportViewer::DrawAllNodes()
	{
		// Only build the data structures once!
		if (m_GraphNeedsRebuild)
			RebuildGraph();

		// Force straight wires (Override bezier curve strength to 0)
		ne::PushStyleVar(ne::StyleVar_LinkStrength, 0.0f);

		// Draw start state node separately since it has a different visual representation
		DrawStartState();

		// Draw all Nodes
		for (auto& [id, node] : m_Nodes)
		{
			auto& state = m_AnimationStateMachine->GetStates().at(id);
			if (!state.PositionSet)
			{
				ne::SetNodePosition(node.ID, ImVec2(state.NodePosition.x, state.NodePosition.y));
				state.PositionSet = true;
			}

			// Draw the node and its pins
			DrawStateNode(node);

			// Check if position is updated by the user
			UpdateNodePositionFromUI(node, state.NodePosition);
		}

		// Draw end state node separately since it has a different visual representation
		DrawEndState();

		// Draw all Links (Transitions)
		for (auto& [id, link] : m_Links)
		{
			DrawTransitionLink(link);
		}

		ne::PopStyleVar();
	}

	void AnimationViewportViewer::DrawStateNode(Node& node)
	{
		ImVec4 nodeBg = node.Color;

		// Slightly darken the node background for better contrast with white text and borders
		nodeBg.x *= 0.8f;
		nodeBg.y *= 0.8f;
		nodeBg.z *= 0.8f;
		nodeBg.w = 1.0f; // Add some transparency for a softer look

		ImVec4 nodeBorder = node.Color;
		nodeBorder.w = 1.0f;

		ne::PushStyleColor(ne::StyleColor_NodeBg, nodeBg);
		ne::PushStyleColor(ne::StyleColor_NodeBorder, nodeBorder);
		ne::PushStyleColor(ne::StyleColor_HovNodeBorder, nodeBorder);
		ne::PushStyleColor(ne::StyleColor_SelNodeBorder, nodeBorder);

		ne::BeginNode(node.ID);

		// Establish the Top-Left origin of the node
		ImVec2 startPos = ImGui::GetCursorScreenPos();

		// Define our constraints
		ImVec2 minSize(120.0f, 40.0f); // Our AAA State Machine block size
		ImVec2 padding(20.0f, 15.0f);  // Extra space just in case the name is huge

		// Measure the text to see if it exceeds our minimums
		ImVec2 textSize = ImGui::CalcTextSize(node.Name.c_str());
		float finalWidth = std::max(minSize.x, textSize.x + padding.x);
		float finalHeight = std::max(minSize.y, textSize.y + padding.y);

		// Center the text perfectly inside our calculated dimensions
		float offsetX = (finalWidth - textSize.x) * 0.5f;
		float offsetY = (finalHeight - textSize.y) * 0.5f;

		ImGui::SetCursorScreenPos(ImVec2(startPos.x + offsetX, startPos.y + offsetY));
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
		ImGui::TextUnformatted(node.Name.c_str());
		ImGui::PopStyleColor();

		// Force the node bounds to expand
		// Move the cursor to the bottom-right corner and drop an invisible element
		ImGui::SetCursorScreenPos(ImVec2(startPos.x + finalWidth, startPos.y + finalHeight));
		ImGui::Dummy(ImVec2(0, 0));

		// Draw the invisible center pins using our new perfect dimensions
		ImVec2 centerPos = ImVec2(startPos.x + (finalWidth * 0.5f), startPos.y + (finalHeight * 0.5f));

		// Draw Input Pin (Center, Invisible anchor point)
		if (node.Inputs.size() > 0)
		{
			ImGui::SetCursorScreenPos(centerPos);
			ne::BeginPin(node.Inputs[0].ID, ne::PinKind::Input);
			ImGui::Dummy(ImVec2(1, 1));
			ne::EndPin();
		}

		// Draw Output Pin (Center, Invisible anchor point)
		if (node.Outputs.size() > 0)
		{
			ImGui::SetCursorScreenPos(centerPos);
			ne::BeginPin(node.Outputs[0].ID, ne::PinKind::Output);
			ImGui::Dummy(ImVec2(1, 1));
			ne::EndPin();
		}

		ne::EndNode();

		ne::PopStyleColor(4);
	}

	void AnimationViewportViewer::DrawTransitionLink(Link& link)
	{
		ne::Link(link.ID, link.StartPinID, link.EndPinID, link.Color, 2.0f);

		// Skip flow for purely visual links (Start -> Default and State -> End)
		bool isVisualLink =
			link.StartPinID.Get() == m_EntryNode.Outputs[0].ID.Get() ||
			link.EndPinID.Get() == m_ExitNode.Inputs[0].ID.Get();

		if (!isVisualLink)
		{
			// Animate a pulsing flow down the wire to visually indicate the direction 
			// of the transition, instantly solving the bidirectional ambiguity problem
			ne::Flow(link.ID);
		}

		// TODO: In the future, will want a preview window and when testing the animation state flow, will provide a "Live flow"
		//  that shows the flow traveling down the wires in real time as the animation plays out (See UE5's Animation State Machine for reference)
	}

	void AnimationViewportViewer::DrawStartState()
	{
		if (!m_AnimationStateMachine->EntryNodePositionSet)
		{
			ne::SetNodePosition(ENTRY_NODE_ID, ImVec2(m_AnimationStateMachine->EntryNodePosition.x, m_AnimationStateMachine->EntryNodePosition.y));
			m_AnimationStateMachine->EntryNodePositionSet = true;
		}

		DrawStateNode(m_EntryNode);

		// Check if position is updated by the user
		UpdateNodePositionFromUI(m_EntryNode, m_AnimationStateMachine->EntryNodePosition);
	}

	void AnimationViewportViewer::DrawEndState()
	{
		if (!m_AnimationStateMachine->ExitNodePositionSet)
		{
			ne::SetNodePosition(EXIT_NODE_ID, ImVec2(m_AnimationStateMachine->ExitNodePosition.x, m_AnimationStateMachine->ExitNodePosition.y));
			m_AnimationStateMachine->ExitNodePositionSet = true;
		}

		DrawStateNode(m_ExitNode);

		// Check if position is updated by the user
		UpdateNodePositionFromUI(m_ExitNode, m_AnimationStateMachine->ExitNodePosition);
	}

	void AnimationViewportViewer::UpdateNodePositionFromUI(Node& node, Vector2f& nodeSavedPosition)
	{
		ImVec2 currentUIPos = ne::GetNodePosition(node.ID);
		if (currentUIPos.x != nodeSavedPosition.x || currentUIPos.y != nodeSavedPosition.y)
		{
			nodeSavedPosition = Vector2f(currentUIPos.x, currentUIPos.y);

			// Mark dirty to re-save the asset with the new position data
			m_SaveCooldown = AUTO_SAVE_DELAY;
			m_IsDirty = true;
		}
	}

	void AnimationViewportViewer::CheckNodeSelected()
	{
		if (!ne::HasSelectionChanged())
			return;

		// Find out how many items (nodes + links) are selected
		int selectedObjectCount = ne::GetSelectedObjectCount();

		std::vector<ne::NodeId> selectedNodes;
		selectedNodes.resize(selectedObjectCount);

		std::vector<ne::LinkId> selectedLinks;
		selectedLinks.resize(selectedObjectCount);

		// Populate the array with only the selected nodes
		int nodeCount = ne::GetSelectedNodes(selectedNodes.data(), selectedNodes.size());
		int linkCount = ne::GetSelectedLinks(selectedLinks.data(), selectedLinks.size());
		if (nodeCount > 0)
		{
			ne::NodeId clickedNode = selectedNodes[0];
			uint64_t nodeId = static_cast<uint64_t>(clickedNode.Get());
			
			if (m_AnimationStateMachine->GetStates().contains(nodeId))
				m_SelectedState = &m_AnimationStateMachine->GetStates().at(nodeId);
			else
				m_SelectedState = nullptr;

			// De-select any transition
			m_SelectedTransition = nullptr;
		}
		else if (linkCount > 0)
		{
			ne::LinkId clickedLink = selectedLinks[0];
			uint64_t linkId = static_cast<uint64_t>(clickedLink.Get());

			m_SelectedTransition = m_AnimationStateMachine->GetTransitionById(linkId);

			// De-select any nodes
			m_SelectedState = nullptr;
		}
		else
		{
			// The user clicked the background and deselected everything
			m_SelectedState = nullptr;
			m_SelectedTransition = nullptr;
		}
	}

	void AnimationViewportViewer::RenderContextMenus()
	{
		// Suspend the node editor so we can draw standard ImGui popups over it
		ne::Suspend();

		// Query the editor to see if the user right-clicked the empty canvas
		if (ne::ShowBackgroundContextMenu())
		{
			ImGui::OpenPopup("AnimationGraphContextMenu");
		}

		// Render node context menu if the user right-clicked a node
		if (ne::ShowNodeContextMenu(&m_NodePopupId))
		{ 
			ImGui::OpenPopup("NodeContextMenu");
		}

		// Draw the popups
		RenderDefaultContextMenu();
		RenderNodeContextMenu(m_NodePopupId);

		// Resume the node editor context
		ne::Resume();
	}

	void AnimationViewportViewer::RenderDefaultContextMenu()
	{
		if (ImGui::BeginPopup("AnimationGraphContextMenu"))
		{
			// Get the screen-space position where the user initially right-clicked
			ImVec2 clickPos = ImGui::GetMousePosOnOpeningCurrentPopup();

			if (ImGui::MenuItem("Create New State"))
			{
				ImVec2 canvasPos = ne::ScreenToCanvas(clickPos);

				// Ensure unique state name
				std::string newStateName = "New State";
				int suffix = 1;
				while (m_AnimationStateMachine->ContainsState(newStateName))
					newStateName = "New State " + std::to_string(suffix++);

				auto& state = m_AnimationStateMachine->CreateState(newStateName);

				// Set the newly created state's position to the converted canvas coordinates
				state.NodePosition = { canvasPos.x, canvasPos.y };

				// Mark the graph to rebuild and save
				m_GraphNeedsRebuild = true;
				m_IsDirty = true;
				m_SaveCooldown = AUTO_SAVE_DELAY;

				m_SelectedState = &state;
			}

			ImGui::EndPopup();
		}
	}

	void AnimationViewportViewer::RenderNodeContextMenu(ne::NodeId nodeId)
	{
		if (nodeId.Get() == Constants::InvalidUUID)
			return;

		if (ImGui::BeginPopup("NodeContextMenu"))
		{
			UUID nodeIdValue = static_cast<UUID>(nodeId.Get());
			if (m_AnimationStateMachine->GetStates().contains(nodeIdValue))
			{
				// Context menu for State Nodes
				if (ImGui::MenuItem("Create Transition"))
				{
					m_InteractiveTransitionOrigin = nodeId;
				}

				if (ImGui::MenuItem("Delete State"))
				{
					DeleteNode(nodeIdValue);
				}
			}

			ImGui::EndPopup();
		}
	}

	void AnimationViewportViewer::HandleInteractiveTransition()
	{
		// Bail out if we aren't actively drawing a transition
		UUID originId = m_InteractiveTransitionOrigin.Get();
		if (originId == Constants::InvalidUUID)
			return;

		// 1. Find the exact center of the Origin Node in Canvas space
		ImVec2 nodePos = ne::GetNodePosition(m_InteractiveTransitionOrigin);
		ImVec2 nodeSize = ne::GetNodeSize(m_InteractiveTransitionOrigin);
		ImVec2 centerCanvasPos = ImVec2(nodePos.x + (nodeSize.x * 0.5f), nodePos.y + (nodeSize.y * 0.5f));

		// 2. Convert to Screen space and cache it for the drawing phase outside ne::End()
		m_InteractiveTransitionScreenStart = ne::CanvasToScreen(centerCanvasPos);

		// 3. Handle Completion Click
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			ne::NodeId hoveredNodeId = ne::GetHoveredNode();
			if (hoveredNodeId)
			{
				UUID targetId = static_cast<UUID>(hoveredNodeId.Get());

				// Make sure we didn't click ourselves, and the target is a valid state
				if (targetId != originId && m_AnimationStateMachine->GetStates().contains(targetId))
				{
					// Add the transition to the animation state machine
					m_AnimationStateMachine->CreateTransition(originId, targetId);

					m_GraphNeedsRebuild = true;
					m_IsDirty = true;
					m_SaveCooldown = AUTO_SAVE_DELAY;
				}
			}

			// Regardless of whether they clicked a node or the background, exit the drawing state
			m_InteractiveTransitionOrigin = Constants::InvalidUUID;
		}

		// 4. Handle Cancellation
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || Input::IsKeyPressed(KeyCode::Escape))
		{
			m_InteractiveTransitionOrigin = Constants::InvalidUUID;
		}
	}

	void AnimationViewportViewer::DeleteNode(UUID nodeId)
	{
		m_AnimationStateMachine->RemoveState(nodeId);
		m_Nodes.erase(nodeId);
		m_SelectedState = nullptr;

		m_GraphNeedsRebuild = true;
		MarkAnimationStateMachineDirty();
	}

	void AnimationViewportViewer::DeleteTransition(UUID transitionId)
	{
		m_AnimationStateMachine->RemoveTransition(transitionId);
		m_Links.erase(transitionId);
		m_SelectedTransition = nullptr;

		m_GraphNeedsRebuild = true;
		MarkAnimationStateMachineDirty();
	}

}