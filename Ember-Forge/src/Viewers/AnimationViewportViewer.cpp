#include "efpch.h"

#include "AnimationViewportViewer.h"
#include "EditorLayer.h"

#include <imgui/imgui.h>

namespace Ember {
	AnimationViewportViewer::AnimationViewportViewer(SharedPtr<Scene> scene, const std::string& filePath, const std::string& title)
		: EditorViewportViewer(Type::Animation, scene, filePath, title) {
	}

	void AnimationViewportViewer::OnImGuiRender(EditorLayer* editor)
	{
		editor->SetViewportHovered(ImGui::IsWindowHovered());
		editor->SetViewportFocused(ImGui::IsWindowFocused());

		// TODO: Setup ed::SetCurrentEditor(m_NodeEditorContext);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		ImGui::Text("Animation Node Editor Context Will Render Here");

		// ed::Begin("Animation Graph");
		// ed::End();
	}
}