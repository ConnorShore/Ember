#pragma once
#include "EditorViewportViewer.h"
#include "GraphNodes.h"
#include <Ember/Animation/AnimationController.h>
#include <Ember/Animation/AnimationTransition.h>
#include <imgui_node_editor.h>
#include <unordered_map>
#include <string>

namespace ne = ax::NodeEditor;

namespace Ember {

	const float AUTO_SAVE_DELAY = 3.0f; // Wait time in seconds after a change before auto-saving the asset

	class EditorContext;

	class AnimationViewportViewer final : public EditorViewportViewer
	{
	public:
		AnimationViewportViewer(SharedPtr<Scene> scene, SharedPtr<AnimationController> animationController, const std::string& filePath, const std::string& title);
		virtual ~AnimationViewportViewer();

		virtual void OnOpen(EditorLayer* editor) override;
		virtual void OnUpdate(TimeStep delta, EditorLayer* editor) override;
		virtual void OnImGuiRender(EditorLayer* editor) override;

		void SaveAnimationStateMachine(EditorLayer* editor);
		void DeleteNode(UUID nodeId);
		void DeleteTransition(UUID transitionId);

		inline void MarkAnimationStateMachineDirty()
		{
			m_SaveCooldown = AUTO_SAVE_DELAY;
			m_IsDirty = true;
		}

		inline void RenameNode(UUID stateId, const std::string& name) { m_Nodes[stateId].Name = name.c_str(); }

		void SetSelectedTransition(AnimationTransition* transition)
		{
			m_SelectedTransition = transition;
			m_SelectedState = nullptr;
		}

		inline AnimationState* GetSelectedState() { return m_SelectedState; }
		inline AnimationTransition* GetSelectedTransition() { return m_SelectedTransition; }
		inline SharedPtr<AnimationController> GetAnimationController() { return m_AnimationController; }
		AnimationStateMachine* GetEditableStateMachine();

	private:
		void DrawLayerPanel();
		void RebuildGraph();
		void HandleHotkeys();
		void DrawAllNodes();
		void DrawStateNode(Node& node);
		void DrawTransitionLink(Link& link);
		void StartTransitionCreation(ne::NodeId originNodeId);

		void DrawStartState();
		void DrawEndState();

		void UpdateNodePositionFromUI(Node& node, Vector2f& nodeSavedPosition);

		void CheckNodeSelected();

		void HandleContextMenuQueries();
		void RenderDefaultContextMenu();
		void RenderNodeContextMenu(ne::NodeId nodeId);
		void RenderLinkContextMenu(ne::LinkId linkId);
		void HandleInteractiveTransition();

	private:
		const uint64_t ENTRY_NODE_ID = 0xFFFFFFFFFFFFFFFE;
		const uint64_t ENTRY_PIN_ID = 0xFFFFFFFFFFFFFFFD;
		const uint64_t ENTRY_LINK_ID = 0xFFFFFFFFFFFFFFFC;

		const uint64_t EXIT_NODE_ID = 0xFFFFFFFFFFFFFFFB;
		const uint64_t EXIT_PIN_ID = 0xFFFFFFFFFFFFFFFA;
		const uint64_t EXIT_LINK_ID = 0xFFFFFFFFFFFFFFF9;

	private:
		ax::NodeEditor::EditorContext* m_NodeEditorContext = nullptr;
		SharedPtr<AnimationController> m_AnimationController;

		// Layer Management
		int m_ActiveLayerIndex = 0;
		int m_RenamingLayerIndex = -1;
		std::string m_LayerRenameBuffer;
		bool m_FocusLayerRename = false;

		bool m_GraphNeedsRebuild = true;

		// Keyed by StateId to easily resolve Transitions
		std::unordered_map<UUID, Node> m_Nodes;
		std::unordered_map<uint64_t, Link> m_Links;

		Node m_EntryNode;
		Node m_ExitNode;

		bool m_IsDirty = false; // Set to true when the graph has been modified and needs to be save
		float m_SaveCooldown = 0.0f;

		AnimationState* m_SelectedState = nullptr;
		AnimationTransition* m_SelectedTransition = nullptr;

		UUID m_InteractiveTransitionOrigin = Constants::InvalidUUID;
		ImVec2 m_InteractiveTransitionScreenStart;

		// Context Menu Data
		ne::NodeId m_NodePopupId = 0;
		ne::LinkId m_LinkPopupId = 0;
		ImVec2 m_ContextPopupMousePos;
		ImVec2 m_ContextPopupCanvasPos;
		bool m_PositionDefaultContextMenu = false;

		// Deferred Popup Triggers
		bool m_RequestDefaultContextMenu = false;
		bool m_RequestNodeContextMenu = false;
		bool m_RequestLinkContextMenu = false;
	};
}