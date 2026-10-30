#pragma once

#include <Ember/Math/Math.h>

namespace Ember {

	class EditorLayer;

}

namespace Ember::UI {

	enum class ViewportOverlayAnchor
	{
		TopLeft = 0,
		TopRight,
		BottomLeft,
		BottomRight
	};

	struct ViewportImageRect
	{
		Vector2f Min;
		Vector2f Max;
	};

	struct ViewportOverlayPlacement
	{
		Vector2f Position = { 10.0f, 10.0f };	// Inset from the anchored corner, in pixels
		Vector2f Size = { 0.0f, 0.0f };			// Zero on an axis auto-sizes that axis to the contents
		ViewportOverlayAnchor Anchor = ViewportOverlayAnchor::TopRight;
	};

	class ViewportOverlay
	{
	public:
		virtual ~ViewportOverlay() = default;

		// Returns the size it occupied, so a caller stacking overlays can offset the next one.
		Vector2f Render(EditorLayer* editor, const ViewportOverlayPlacement& placement);

	protected:
		virtual const char* GetWindowID() const = 0;
		virtual void OnImGuiRender(EditorLayer* editor) = 0;

		virtual bool ShouldDraw(EditorLayer* editor) const { return true; }
		virtual float GetBackgroundAlpha() const { return 0.35f; }
		virtual Vector2f GetWindowPadding() const;
	};

}
