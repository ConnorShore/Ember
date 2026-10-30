#include "efpch.h"

#include "ViewportOverlay.h"
#include "EditorLayer.h"

namespace Ember::UI {

	Vector2f ViewportOverlay::GetWindowPadding() const
	{
		const ImVec2& padding = ImGui::GetStyle().WindowPadding;
		return { padding.x, padding.y };
	}

	Vector2f ViewportOverlay::Render(EditorLayer* editor, const ViewportOverlayPlacement& placement)
	{
		if (!editor || !ShouldDraw(editor))
			return { 0.0f, 0.0f };

		const ViewportImageRect imageRect = editor->GetViewportImageRect();

		bool anchorRight = placement.Anchor == ViewportOverlayAnchor::TopRight || placement.Anchor == ViewportOverlayAnchor::BottomRight;
		bool anchorBottom = placement.Anchor == ViewportOverlayAnchor::BottomLeft || placement.Anchor == ViewportOverlayAnchor::BottomRight;

		ImVec2 pivot(anchorRight ? 1.0f : 0.0f, anchorBottom ? 1.0f : 0.0f);
		ImVec2 windowPos(
			anchorRight ? imageRect.Max.x - placement.Position.x : imageRect.Min.x + placement.Position.x,
			anchorBottom ? imageRect.Max.y - placement.Position.y : imageRect.Min.y + placement.Position.y
		);

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

		if (placement.Size.x > 0.0f || placement.Size.y > 0.0f)
			ImGui::SetNextWindowSize(ImVec2(placement.Size.x, placement.Size.y), ImGuiCond_Always);
		else
			windowFlags |= ImGuiWindowFlags_AlwaysAutoResize;

		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, pivot);
		ImGui::SetNextWindowBgAlpha(GetBackgroundAlpha());

		Vector2f padding = GetWindowPadding();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding.x, padding.y));

		if (ImGui::Begin(GetWindowID(), nullptr, windowFlags))
			OnImGuiRender(editor);

		ImVec2 windowSize = ImGui::GetWindowSize();

		ImGui::End();
		ImGui::PopStyleVar();

		return { windowSize.x, windowSize.y };
	}

}
