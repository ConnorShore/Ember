#include "efpch.h"

#include "ProjectionOverlay.h"
#include "EditorLayer.h"

#include <Ember/Core/Paths.h>
#include <Ember/Render/Texture2D.h>

namespace Ember::UI {

	uint32_t ProjectionOverlay::s_PerspectiveTextureID = 0;
	uint32_t ProjectionOverlay::s_OrthographicTextureID = 0;

	void ProjectionOverlay::LoadResources()
	{
		const std::filesystem::path iconDir = Paths::EditorAssets() / "icons";
		auto& assetManager = Application::Instance().GetAssetManager();

		s_PerspectiveTextureID = assetManager.Load<Texture2D>((iconDir / "PerspectiveGrid.png").string())->GetID();
		s_OrthographicTextureID = assetManager.Load<Texture2D>((iconDir / "OrthographicGrid.png").string())->GetID();
	}

	Vector2f ProjectionOverlay::Draw(EditorLayer* editor, const ViewportOverlayPlacement& placement)
	{
		ProjectionOverlay overlay;
		return overlay.Render(editor, placement);
	}

	bool ProjectionOverlay::ShouldDraw(EditorLayer* editor) const
	{
		return editor->GetContext().CurrentSceneState == SceneState::Edit;
	}

	void ProjectionOverlay::OnImGuiRender(EditorLayer* editor)
	{
		EditorCamera& camera = editor->GetCamera();

		bool isOrthographic = camera.IsOrthographic();
		uint32_t icon = isOrthographic ? s_OrthographicTextureID : s_PerspectiveTextureID;

		// Textures are flipped on load, so the UVs put the icon back upright.
		if (ImGui::ImageButton("ProjectionToggle", icon, ImVec2(22.0f, 22.0f), ImVec2(0, 1), ImVec2(1, 0)))
			camera.ToggleProjectionMode();

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(isOrthographic ? "Orthographic - click for perspective" : "Perspective - click for orthographic");
	}

}
