#include "efpch.h"
#include "AnimationViewportViewer.h"
#include "EditorLayer.h"

#include <Ember/Core/ProjectManager.h>
#include <Ember/Animation/AnimationControllerSerializer.h>
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

	AnimationViewportViewer::AnimationViewportViewer(SharedPtr<Scene> scene, SharedPtr<AnimationController> animationController, const std::string& filePath, const std::string& title)
		: EditorViewportViewer(Type::Animation, scene, filePath, title), m_AnimationController(animationController)
	{
		if (m_AnimationController && m_AnimationController->GetLayers().empty())
			m_AnimationController->GetLayers().emplace_back();

		ne::Config config;
		std::string path = (ProjectManager::GetActive()->GetProjectDirectory() / "AnimationNodeEditor.json").string();
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

	AnimationStateMachine* AnimationViewportViewer::GetEditableStateMachine()
	{
		if (!m_AnimationController)
			return nullptr;

		auto& layers = m_AnimationController->GetLayers();
		if (layers.empty())
			layers.emplace_back();

		if (m_ActiveLayerIndex >= layers.size())
			m_ActiveLayerIndex = static_cast<int>(layers.size()) - 1;

		return &layers[m_ActiveLayerIndex].StateMachine;
	}

	void AnimationViewportViewer::OnOpen(EditorLayer* editor)
	{
		auto* stateMachine = GetEditableStateMachine();
		if (!stateMachine)
			return;

		// Mark all states to set position
		stateMachine->EntryNodePositionSet = false;
		stateMachine->ExitNodePositionSet = false;

		for (auto& [stateId, state] : stateMachine->GetStates())
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
		auto* stateMachine = GetEditableStateMachine();
		if (!stateMachine)
			return;

		editor->SetViewportHovered(ImGui::IsWindowHovered());
		editor->SetViewportFocused(ImGui::IsWindowFocused());

		// Layer Selection Panel (Left Panel)
		if (ImGui::BeginChild("LayersPanel", ImVec2(250, 0), true))
		{
			DrawLayerPanel();
		}
		ImGui::EndChild();

		ImGui::SameLine();

		// Node Editor Canvas (Right Panel)
		if (ImGui::BeginChild("NodeCanvas", ImVec2(0, 0), false))
		{
			ne::SetCurrentEditor(m_NodeEditorContext);
			ne::Begin(std::format("##AnimationGraph_{}", GetTitle()).c_str());

			HandleHotkeys();
			DrawAllNodes();

			// These ONLY execute logic/math. They do NOT draw or open popups!
			HandleInteractiveTransition();
			HandleContextMenuQueries();

			ne::End();

			// Draw the interactive transition wire
			if (m_InteractiveTransitionOrigin != Constants::InvalidUUID)
			{
				ImDrawList* drawList = ImGui::GetWindowDrawList();
				drawList->AddLine(m_InteractiveTransitionScreenStart, ImGui::GetMousePos(), IM_COL32(240, 120, 30, 255), 3.0f);
			}

			// Open popups safely in the current ID scope
			if (m_RequestDefaultContextMenu)
			{
				if (m_PositionDefaultContextMenu)
					m_ContextPopupMousePos = ImGui::GetMousePos();

				ImGui::OpenPopup("AnimationGraphContextMenu");
				m_RequestDefaultContextMenu = false;
			}
			if (m_RequestNodeContextMenu)
			{
				ImGui::OpenPopup("NodeContextMenu");
				m_RequestNodeContextMenu = false;
			}
			if (m_RequestLinkContextMenu)
			{
				ImGui::OpenPopup("LinkContextMenu");
				m_RequestLinkContextMenu = false;
			}

			// Render the popups
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
			RenderDefaultContextMenu();
			RenderNodeContextMenu(m_NodePopupId);
			RenderLinkContextMenu(m_LinkPopupId);
			ImGui::PopStyleVar();

			// See if any nodes have been selected
			CheckNodeSelected();

			ne::SetCurrentEditor(nullptr);
		}
		ImGui::EndChild();
	}

	void AnimationViewportViewer::HandleHotkeys()
	{
		if (Input::IsKeyPressed(KeyCode::Space))
		{
			// Spacebar should open the menu at the cursor.
			m_PositionDefaultContextMenu = true;
			m_RequestDefaultContextMenu = true;
		}

		// Check for delete key to delete selected node or link
		if (Input::IsKeyPressed(KeyCode::Delete))
		{
			if (m_SelectedState)
				DeleteNode(m_SelectedState->Id);
			else if (m_SelectedTransition)
				DeleteTransition(m_SelectedTransition->Id);
		}
	}

	void AnimationViewportViewer::HandleContextMenuQueries()
	{
		if (ne::ShowBackgroundContextMenu())
		{
			m_ContextPopupMousePos = ImGui::GetMousePos();
			m_ContextPopupCanvasPos = ne::ScreenToCanvas(m_ContextPopupMousePos);
			m_PositionDefaultContextMenu = false;
			m_RequestDefaultContextMenu = true;
		}

		if (ne::ShowNodeContextMenu(&m_NodePopupId))
		{
			m_RequestNodeContextMenu = true;
		}

		if (ne::ShowLinkContextMenu(&m_LinkPopupId))
		{
			m_RequestLinkContextMenu = true;
		}
	}

	void AnimationViewportViewer::SaveAnimationStateMachine(EditorLayer* editor)
	{
		AnimationControllerSerializer::Serialize(GetFilePath(), m_AnimationController);
		m_IsDirty = false;

		auto evt = UINotificationEvent("Auto-Saved Animation Controller", UINotificationEvent::Severity::Info);
		editor->GetContext().EventCallback(evt);
	}

	void AnimationViewportViewer::DrawLayerPanel()
	{
		ImGui::Text("Animation Layers");
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - 24.0f);

		// Add Layer Button
		if (ImGui::Button("+##AddLayer", ImVec2(24, 24)))
		{
			auto& newLayer = m_AnimationController->CreateLayer("New Layer");
			m_ActiveLayerIndex = static_cast<int>(m_AnimationController->GetLayers().size()) - 1;

			// Start renaming the new layer immediately
			m_RenamingLayerIndex = static_cast<int>(m_AnimationController->GetLayers().size()) - 1;
			m_LayerRenameBuffer = newLayer.Name;
			m_FocusLayerRename = true; // Tell ImGui to auto-focus the text box next frame

			m_IsDirty = true;
		}

		ImGui::Separator();
		ImGui::Spacing();

		auto& layers = m_AnimationController->GetLayers();
		for (int i = 0; i < layers.size(); ++i)
		{
			auto& layer = layers[i];

			ImGui::PushID(i);

			// If this layer is being renamed, show an input box instead of regular text
			if (m_RenamingLayerIndex == i)
			{
				char renameBuf[256];
				strncpy_s(renameBuf, sizeof(renameBuf), m_LayerRenameBuffer.c_str(), _TRUNCATE);

				// Force the cursor into this box if it was just created/clicked
				if (m_FocusLayerRename)
				{
					ImGui::SetKeyboardFocusHere();
					m_FocusLayerRename = false;
				}

				ImGui::SetNextItemWidth(-FLT_MIN);

				// Commit the name change if the user presses Enter or clicks away
				if (ImGui::InputText("##LayerRename", renameBuf, sizeof(renameBuf), ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::IsItemDeactivatedAfterEdit())
				{
					std::string newName = renameBuf;
					if (!newName.empty())
					{
						layer.Name = newName;
						m_IsDirty = true;
					}
					m_RenamingLayerIndex = -1; // Exit rename state
				}
				else
				{
					m_LayerRenameBuffer = renameBuf; // Keep buffer updated while typing
				}
			}
			else
			{
				// Regular layer display
				bool isSelected = (m_ActiveLayerIndex == i);
				if (ImGui::Selectable(layer.Name.c_str(), isSelected))
				{
					m_ActiveLayerIndex = i;

					//ne::ClearSelection();
					m_SelectedState = nullptr;
					m_SelectedTransition = nullptr;
					m_GraphNeedsRebuild = true;
				}

				// Double-click to rename
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					m_RenamingLayerIndex = i;
					m_LayerRenameBuffer = layer.Name;
					m_FocusLayerRename = true;
				}

				// Right-click context menu
				if (ImGui::BeginPopupContextItem("LayerContextMenu"))
				{
					if (ImGui::MenuItem("Rename Layer"))
					{
						m_RenamingLayerIndex = i;
						m_LayerRenameBuffer = layer.Name;
						m_FocusLayerRename = true;
					}

					if (i > 0 && ImGui::MenuItem("Delete Layer"))
					{
						layers.erase(layers.begin() + i);

						if (m_ActiveLayerIndex >= layers.size())
							m_ActiveLayerIndex = static_cast<int>(layers.size()) - 1;

						// If we deleted the layer we were renaming, clear the state
						if (m_RenamingLayerIndex == i)
							m_RenamingLayerIndex = -1;

						m_GraphNeedsRebuild = true;
						m_IsDirty = true;
					}
					ImGui::EndPopup();
				}
			}

			ImGui::PopID();
		}
	}

	void AnimationViewportViewer::RebuildGraph()
	{
		auto* stateMachine = GetEditableStateMachine();
		if (!stateMachine)
			return;

		m_Nodes.clear();
		m_Links.clear();

		// Build all Nodes
		for (auto& [stateId, state] : stateMachine->GetStates())
		{
			m_Nodes[stateId] = Node(stateId, state.Name.c_str(), ImColor(64, 128, 255));
			Node& node = m_Nodes[stateId];

			node.Inputs.emplace_back(GetNextId(), "", PinType::Flow);
			node.Outputs.emplace_back(GetNextId(), "", PinType::Flow);

			node.Inputs[0].Node = &node;
			node.Outputs[0].Node = &node;
		}

		// Build all Links (Transitions)
		for (auto& [stateId, transitions] : stateMachine->GetTransitions())
		{
			if (m_Nodes.find(stateId) == m_Nodes.end()) continue;
			Node& startNode = m_Nodes[stateId];

			for (auto& transition : transitions)
			{
				if (m_Nodes.find(transition.ToStateId) == m_Nodes.end())
					continue;

				Node& endNode = m_Nodes[transition.ToStateId];
				uint64_t linkId = transition.Id;
				m_Links[linkId] = Link(linkId, startNode.Outputs[0].ID, endNode.Inputs[0].ID);
			}
		}

		// Connect Entry Node to Default State
		m_Links[ENTRY_LINK_ID] = Link(ENTRY_LINK_ID,
			m_EntryNode.Outputs[0].ID,
			m_Nodes[stateMachine->GetDefaultState()].Inputs[0].ID,
			ImColor(30, 190, 30)
		);

		// Connect all States with no outgoing transitions to the Exit Node
		for (auto& [stateId, node] : m_Nodes)
		{
			if (stateMachine->GetTransitions().find(stateId) == stateMachine->GetTransitions().end())
			{
				uint64_t linkId = GetNextId();
				m_Links[linkId] = Link(linkId, node.Outputs[0].ID, m_ExitNode.Inputs[0].ID, ImColor(190, 30, 30));
			}
		}

		m_GraphNeedsRebuild = false;
	}

	void AnimationViewportViewer::DrawAllNodes()
	{
		auto* stateMachine = GetEditableStateMachine();
		if (!stateMachine)
			return;

		if (m_GraphNeedsRebuild)
			RebuildGraph();

		ne::PushStyleVar(ne::StyleVar_LinkStrength, 0.0f);

		DrawStartState();

		for (auto& [id, node] : m_Nodes)
		{
			if (!stateMachine->GetStates().contains(id))
				continue;

			auto& state = stateMachine->GetStates().at(id);
			if (!state.PositionSet)
			{
				ne::SetNodePosition(node.ID, ImVec2(state.NodePosition.x, state.NodePosition.y));
				state.PositionSet = true;
			}

			DrawStateNode(node);
			UpdateNodePositionFromUI(node, state.NodePosition);
		}

		DrawEndState();

		for (auto& [id, link] : m_Links)
		{
			DrawTransitionLink(link);
		}

		ne::PopStyleVar();
	}

	void AnimationViewportViewer::DrawStateNode(Node& node)
	{
		auto* stateMachine = GetEditableStateMachine();
		if (!stateMachine)
			return;

		ImVec4 nodeBg = node.Color;
		nodeBg.x *= 0.8f;
		nodeBg.y *= 0.8f;
		nodeBg.z *= 0.8f;
		nodeBg.w = 1.0f;

		ImVec4 nodeBorder = node.Color;
		nodeBorder.w = 1.0f;

		ne::PushStyleColor(ne::StyleColor_NodeBg, nodeBg);
		ne::PushStyleColor(ne::StyleColor_NodeBorder, nodeBorder);
		ne::PushStyleColor(ne::StyleColor_HovNodeBorder, nodeBorder);
		ne::PushStyleColor(ne::StyleColor_SelNodeBorder, nodeBorder);

		ne::BeginNode(node.ID);

		ImVec2 startPos = ImGui::GetCursorScreenPos();
		ImVec2 minSize(120.0f, 40.0f);
		ImVec2 padding(20.0f, 15.0f);

		ImVec2 textSize = ImGui::CalcTextSize(node.Name.c_str());
		float finalWidth = std::max(minSize.x, textSize.x + padding.x);
		float finalHeight = std::max(minSize.y, textSize.y + padding.y);

		float offsetX = (finalWidth - textSize.x) * 0.5f;
		float offsetY = (finalHeight - textSize.y) * 0.5f;

		ImGui::SetCursorScreenPos(ImVec2(startPos.x + offsetX, startPos.y + offsetY));
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
		ImGui::TextUnformatted(node.Name.c_str());
		ImGui::PopStyleColor();

		ImGui::SetCursorScreenPos(ImVec2(startPos.x + finalWidth, startPos.y + finalHeight));
		ImGui::Dummy(ImVec2(0, 0));

		ImVec2 centerPos = ImVec2(startPos.x + (finalWidth * 0.5f), startPos.y + (finalHeight * 0.5f));

		if (node.Inputs.size() > 0)
		{
			ImGui::SetCursorScreenPos(centerPos);
			ne::BeginPin(node.Inputs[0].ID, ne::PinKind::Input);
			ImGui::Dummy(ImVec2(1, 1));
			ne::EndPin();
		}

		if (node.Outputs.size() > 0)
		{
			ImGui::SetCursorScreenPos(centerPos);
			ne::BeginPin(node.Outputs[0].ID, ne::PinKind::Output);
			ImGui::Dummy(ImVec2(1, 1));
			ne::EndPin();
		}

		bool isTransitionMode = (m_InteractiveTransitionOrigin != Constants::InvalidUUID) || ImGui::GetIO().KeyCtrl;
		if (isTransitionMode)
		{
			ImGui::SetCursorScreenPos(startPos);
			ImGui::PushID(static_cast<int>(node.ID.Get()));

			ImGui::InvisibleButton("##NodeBlocker", ImVec2(finalWidth, finalHeight));

			bool isHovered = ImGui::IsItemHovered();
			bool isClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left); // Triggers instantly on press
			bool isReleased = isHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left); // Triggers on let-go

			UUID nodeIdValue = static_cast<UUID>(node.ID.Get());

			// If we're not currently drawing a transition, and they hold Ctrl and click a node, start drawing a transition from that node
			if (m_InteractiveTransitionOrigin == Constants::InvalidUUID && ImGui::GetIO().KeyCtrl)
			{
				if (isClicked) // Line starts the exact frame the mouse goes down
				{
					if (stateMachine->GetStates().contains(nodeIdValue))
						StartTransitionCreation(node.ID);
				}
			}
			else if (m_InteractiveTransitionOrigin != Constants::InvalidUUID)
			{
				// Triggers if they click the target, OR if they drag and release on the target
				if (isClicked || isReleased)
				{
					UUID originId = m_InteractiveTransitionOrigin;

					// If they clicked/released the origin node, ignore it so they can keep drawing!
					if (nodeIdValue != originId)
					{
						if (stateMachine->GetStates().contains(nodeIdValue))
						{
							stateMachine->CreateTransition(originId, nodeIdValue);
							m_GraphNeedsRebuild = true;
							m_IsDirty = true;
							m_SaveCooldown = AUTO_SAVE_DELAY;
						}
						// Exit drawing mode
						m_InteractiveTransitionOrigin = Constants::InvalidUUID;
					}
				}
			}

			if (isHovered)
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			}

			ImGui::PopID();
		}

		ne::EndNode();
		ne::PopStyleColor(4);
	}

	void AnimationViewportViewer::DrawTransitionLink(Link& link)
	{
		ne::Link(link.ID, link.StartPinID, link.EndPinID, link.Color, 2.0f);

		bool isVisualLink =
			link.StartPinID.Get() == m_EntryNode.Outputs[0].ID.Get() ||
			link.EndPinID.Get() == m_ExitNode.Inputs[0].ID.Get();

		if (!isVisualLink)
		{
			ne::Flow(link.ID);
		}
	}

	void AnimationViewportViewer::StartTransitionCreation(ne::NodeId originNodeId)
	{
		// FIXED: Extract the raw ID to assign to the UUID variable
		m_InteractiveTransitionOrigin = static_cast<UUID>(originNodeId.Get());
	}

	void AnimationViewportViewer::DrawStartState()
	{
		auto* stateMachine = GetEditableStateMachine();
		if (!stateMachine)
			return;

		if (!stateMachine->EntryNodePositionSet)
		{
			ne::SetNodePosition(ENTRY_NODE_ID, ImVec2(stateMachine->EntryNodePosition.x, stateMachine->EntryNodePosition.y));
			stateMachine->EntryNodePositionSet = true;
		}

		DrawStateNode(m_EntryNode);
		UpdateNodePositionFromUI(m_EntryNode, stateMachine->EntryNodePosition);
	}

	void AnimationViewportViewer::DrawEndState()
	{
		auto* stateMachine = GetEditableStateMachine();
		if (!stateMachine)
			return;

		if (!stateMachine->ExitNodePositionSet)
		{
			ne::SetNodePosition(EXIT_NODE_ID, ImVec2(stateMachine->ExitNodePosition.x, stateMachine->ExitNodePosition.y));
			stateMachine->ExitNodePositionSet = true;
		}

		DrawStateNode(m_ExitNode);
		UpdateNodePositionFromUI(m_ExitNode, stateMachine->ExitNodePosition);
	}

	void AnimationViewportViewer::UpdateNodePositionFromUI(Node& node, Vector2f& nodeSavedPosition)
	{
		// If we are actively drawing a transition or holding the Ctrl key, lock the node
		if (m_InteractiveTransitionOrigin != Constants::InvalidUUID || ImGui::GetIO().KeyCtrl)
		{
			// Force the Node Editor to keep the node exactly where it was saved in our backend
			ne::SetNodePosition(node.ID, ImVec2(nodeSavedPosition.x, nodeSavedPosition.y));
			return;
		}

		ImVec2 currentUIPos = ne::GetNodePosition(node.ID);
		if (currentUIPos.x != nodeSavedPosition.x || currentUIPos.y != nodeSavedPosition.y)
		{
			nodeSavedPosition = Vector2f(currentUIPos.x, currentUIPos.y);
			m_SaveCooldown = AUTO_SAVE_DELAY;
			m_IsDirty = true;
		}
	}

	void AnimationViewportViewer::CheckNodeSelected()
	{
		auto* stateMachine = GetEditableStateMachine();
		if (!stateMachine)
			return;

		if (!ne::HasSelectionChanged())
			return;

		int selectedObjectCount = ne::GetSelectedObjectCount();
		std::vector<ne::NodeId> selectedNodes;
		selectedNodes.resize(selectedObjectCount);

		std::vector<ne::LinkId> selectedLinks;
		selectedLinks.resize(selectedObjectCount);

		int nodeCount = ne::GetSelectedNodes(selectedNodes.data(), selectedNodes.size());
		int linkCount = ne::GetSelectedLinks(selectedLinks.data(), selectedLinks.size());

		if (nodeCount > 0)
		{
			ne::NodeId clickedNode = selectedNodes[0];
			uint64_t nodeId = static_cast<uint64_t>(clickedNode.Get());

			if (stateMachine->GetStates().contains(nodeId))
				m_SelectedState = &stateMachine->GetStates().at(nodeId);
			else
				m_SelectedState = nullptr;

			m_SelectedTransition = nullptr;
		}
		else if (linkCount > 0)
		{
			ne::LinkId clickedLink = selectedLinks[0];
			uint64_t linkId = static_cast<uint64_t>(clickedLink.Get());

			m_SelectedTransition = stateMachine->GetTransitionById(linkId);
			m_SelectedState = nullptr;
		}
		else
		{
			m_SelectedState = nullptr;
			m_SelectedTransition = nullptr;
		}
	}

	void AnimationViewportViewer::RenderDefaultContextMenu()
	{
		auto* stateMachine = GetEditableStateMachine();
		if (!stateMachine)
			return;

		if (m_PositionDefaultContextMenu)
			ImGui::SetNextWindowPos(m_ContextPopupMousePos, ImGuiCond_Appearing);

		if (ImGui::BeginPopup("AnimationGraphContextMenu"))
		{
			ImVec2 popupOpenMousePos = m_PositionDefaultContextMenu
				? m_ContextPopupMousePos
				: ImGui::GetMousePosOnOpeningCurrentPopup();
			ImVec2 popupCanvasPos = ne::ScreenToCanvas(popupOpenMousePos);

			m_PositionDefaultContextMenu = false;

			if (ImGui::MenuItem("Create New State"))
			{
				ImVec2 canvasPos = popupCanvasPos;

				std::string newStateName = "New State";
				int suffix = 1;
				while (stateMachine->ContainsState(newStateName))
					newStateName = "New State " + std::to_string(suffix++);

				auto& state = stateMachine->CreateState(newStateName);
				state.NodePosition = { canvasPos.x, canvasPos.y };

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
		auto* stateMachine = GetEditableStateMachine();
		if (!stateMachine)
			return;

		if (nodeId.Get() == Constants::InvalidUUID)
			return;

		if (ImGui::BeginPopup("NodeContextMenu"))
		{
			UUID nodeIdValue = static_cast<UUID>(nodeId.Get());
			if (stateMachine->GetStates().contains(nodeIdValue))
			{
				if (ImGui::MenuItem("Create Transition"))
				{
					StartTransitionCreation(nodeId);
				}

				if (ImGui::MenuItem("Delete State"))
				{
					DeleteNode(nodeIdValue);
				}
			}

			ImGui::EndPopup();
		}
	}

	void AnimationViewportViewer::RenderLinkContextMenu(ne::LinkId linkId)
	{
		auto* stateMachine = GetEditableStateMachine();
		if (!stateMachine)
			return;

		if (linkId.Get() == Constants::InvalidUUID)
			return;

		if (ImGui::BeginPopup("LinkContextMenu"))
		{
			UUID linkIdValue = static_cast<UUID>(linkId.Get());
			if (stateMachine->GetTransitionById(linkIdValue))
			{
				if (ImGui::MenuItem("Delete Transition"))
				{
					DeleteTransition(linkIdValue);
				}
			}

			ImGui::EndPopup();
		}
	}

	void AnimationViewportViewer::HandleInteractiveTransition()
	{
		UUID originId = m_InteractiveTransitionOrigin;
		if (originId == Constants::InvalidUUID)
			return;

		// Find the exact center of the Origin Node in Canvas space
		ImVec2 nodePos = ne::GetNodePosition(ne::NodeId(originId));
		ImVec2 nodeSize = ne::GetNodeSize(ne::NodeId(originId));
		ImVec2 centerCanvasPos = ImVec2(nodePos.x + (nodeSize.x * 0.5f), nodePos.y + (nodeSize.y * 0.5f));

		// Convert to Screen space and cache it for the drawing phase outside ne::End()
		m_InteractiveTransitionScreenStart = ne::CanvasToScreen(centerCanvasPos);

		// Handle Cancellation
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || Input::IsKeyPressed(KeyCode::Escape))
		{
			m_InteractiveTransitionOrigin = Constants::InvalidUUID;
		}
		else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			// If they clicked the left mouse button, but no node was hovered 
			// (because our InvisibleButtons eat the hover state over nodes), cancel the line!
			if (!ImGui::IsAnyItemHovered())
			{
				m_InteractiveTransitionOrigin = Constants::InvalidUUID;
			}
		}
	}

	void AnimationViewportViewer::DeleteNode(UUID nodeId)
	{
		auto* stateMachine = GetEditableStateMachine();
		if (!stateMachine)
			return;

		stateMachine->RemoveState(nodeId);
		m_Nodes.erase(nodeId);
		m_SelectedState = nullptr;

		m_GraphNeedsRebuild = true;
		MarkAnimationStateMachineDirty();
	}

	void AnimationViewportViewer::DeleteTransition(UUID transitionId)
	{
		auto* stateMachine = GetEditableStateMachine();
		if (!stateMachine)
			return;

		stateMachine->RemoveTransition(transitionId);
		m_Links.erase(transitionId);
		m_SelectedTransition = nullptr;

		m_GraphNeedsRebuild = true;
		MarkAnimationStateMachineDirty();
	}

}