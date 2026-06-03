#pragma once

#include "EditorViewportViewer.h"

#include <Ember/Animation/AnimationStateMachine.h>
#include <imgui_node_editor.h>

namespace Ember {

	class EditorContext;

	class AnimationViewportViewer final : public EditorViewportViewer
	{
	public:
		AnimationViewportViewer(SharedPtr<Scene> scene, SharedPtr<AnimationStateMachine> animationStateMachine, const std::string& filePath, const std::string& title);
		virtual void OnImGuiRender(EditorLayer* editor) override;

	private:
		ax::NodeEditor::EditorContext* m_NodeEditorContext = nullptr;
		SharedPtr<AnimationStateMachine> m_AnimationStateMachine;
	};

}