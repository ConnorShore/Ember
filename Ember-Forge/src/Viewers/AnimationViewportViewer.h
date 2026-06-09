#pragma once
#include "EditorViewportViewer.h"
#include "GraphNodes.h"

#include <Ember/Animation/AnimationStateMachine.h>
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
		AnimationViewportViewer(SharedPtr<Scene> scene, SharedPtr<AnimationStateMachine> animationStateMachine, const std::string& filePath, const std::string& title);
		virtual ~AnimationViewportViewer();

		virtual void OnOpen(EditorLayer* editor) override;
		virtual void OnUpdate(TimeStep delta, EditorLayer* editor) override;
		virtual void OnImGuiRender(EditorLayer* editor) override;

		void SaveAnimationStateMachine(EditorLayer* editor);

		inline void MarkAnimationStateMachineDirty()
		{
			m_SaveCooldown = AUTO_SAVE_DELAY;
			m_IsDirty = true;
		}

		inline void RenameNode(UUID stateId, const std::string& name) { m_Nodes[stateId].Name = name.c_str(); }

		inline AnimationState* GetSelectedState() { return m_SelectedState; }
		inline AnimationTransition* GetSelectedTransition() { return m_SelectedTransition; }
		inline SharedPtr<AnimationStateMachine> GetAnimationStateMachine() { return m_AnimationStateMachine; }

	private:
		void RebuildGraph();
		void HandleHotkeys();
		void DrawAllNodes();
		void DrawStateNode(Node& node);
		void DrawTransitionLink(Link& link);

		void DrawStartState();
		void DrawEndState();

		void UpdateNodePositionFromUI(Node& node, Vector2f& nodeSavedPosition);

		void CheckNodeSelected();

		void RenderContextMenus();
		void RenderDefaultContextMenu();
		void RenderNodeContextMenu(ne::NodeId nodeId);

		void DeleteNode(UUID nodeId);
		void DeleteTransition(UUID transitionId);

	private:
		const uint64_t ENTRY_NODE_ID = 0xFFFFFFFFFFFFFFFE;
		const uint64_t ENTRY_PIN_ID = 0xFFFFFFFFFFFFFFFD;
		const uint64_t ENTRY_LINK_ID = 0xFFFFFFFFFFFFFFFC;

		const uint64_t EXIT_NODE_ID = 0xFFFFFFFFFFFFFFFB;
		const uint64_t EXIT_PIN_ID = 0xFFFFFFFFFFFFFFFA;
		const uint64_t EXIT_LINK_ID = 0xFFFFFFFFFFFFFFF9;

	private:
		ax::NodeEditor::EditorContext* m_NodeEditorContext = nullptr;
		SharedPtr<AnimationStateMachine> m_AnimationStateMachine;

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

		ne::NodeId m_NodePopupId = Constants::InvalidUUID;
	};
}