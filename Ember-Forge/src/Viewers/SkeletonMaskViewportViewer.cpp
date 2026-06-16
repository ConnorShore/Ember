#include "efpch.h"

#include "SkeletonMaskViewportViewer.h"
#include "EditorLayer.h"

#include <Ember/Asset/SkeletonMask.h>
#include <imgui/imgui.h>

namespace Ember {

	SkeletonMaskViewportViewer::SkeletonMaskViewportViewer(SharedPtr<Scene> scene, SharedPtr<SkeletonMask> skeletonMask, const std::string& filePath, const std::string& title)
		: EditorViewportViewer(Type::SkeletonMask, scene, filePath, title), m_SkeletonMask(skeletonMask)
	{
	}

	SkeletonMaskViewportViewer::~SkeletonMaskViewportViewer()
	{

	}

	void SkeletonMaskViewportViewer::OnImGuiRender(EditorLayer* editor)
	{
		editor->SetViewportHovered(ImGui::IsWindowHovered());
		editor->SetViewportFocused(ImGui::IsWindowFocused());

		ImGui::Text("Skeleton Mask Editor - Work in Progress");

		// TODO: Render a side panel with the skeleton hierarchy and sliders for each bone weight, as well as a global weight slider.
		// TODO: Have a viewport section to view the skeleton in default position and visualize the bone weights with colors or something.
	}

}