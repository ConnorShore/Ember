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

	private:
		void RebuildGraph();
		void DrawAllNodes();

	private:
		ax::NodeEditor::EditorContext* m_NodeEditorContext = nullptr;
		SharedPtr<AnimationStateMachine> m_AnimationStateMachine;

		bool m_GraphNeedsRebuild = true;

		// Keyed by StateId to easily resolve Transitions
		std::unordered_map<UUID, Node> m_Nodes;
		std::unordered_map<uint64_t, Link> m_Links;

		bool m_IsDirty = false; // Set to true when the graph has been modified and needs to be save
		float m_SaveCooldown = 0.0f;
	};
}