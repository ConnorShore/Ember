#pragma once

#include "UI/Overlays/ViewportOverlay.h"

namespace Ember::UI {

	class ProjectionOverlay final : public ViewportOverlay
	{
	public:
		static void LoadResources();

		static Vector2f Draw(EditorLayer* editor, const ViewportOverlayPlacement& placement);

	protected:
		const char* GetWindowID() const override { return "Viewport Controls"; }
		void OnImGuiRender(EditorLayer* editor) override;

		bool ShouldDraw(EditorLayer* editor) const override;
		Vector2f GetWindowPadding() const override { return { 4.0f, 4.0f }; }

	private:
		static uint32_t s_PerspectiveTextureID;
		static uint32_t s_OrthographicTextureID;
	};

}
