#include "efpch.h"

#include "AnimationViewportViewer.h"
#include "EditorLayer.h"

#include <imgui/imgui.h>
#include <format>

namespace ed = ax::NodeEditor;

namespace Ember {

	AnimationViewportViewer::AnimationViewportViewer(SharedPtr<Scene> scene, SharedPtr<AnimationStateMachine> animationStateMachine, const std::string& filePath, const std::string& title)
		: EditorViewportViewer(Type::Animation, scene, filePath, title), m_AnimationStateMachine(animationStateMachine){
		m_NodeEditorContext = ed::CreateEditor();
	}

	void AnimationViewportViewer::OnImGuiRender(EditorLayer* editor)
	{
		editor->SetViewportHovered(ImGui::IsWindowHovered());
		editor->SetViewportFocused(ImGui::IsWindowFocused());

		ed::SetCurrentEditor(m_NodeEditorContext);

		ed::Begin(std::format("##AnimationGraph_{}", m_AnimationStateMachine->GetName()).c_str());

		ed::End();
	}
}