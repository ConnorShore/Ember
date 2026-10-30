#pragma once

#include "UI/Overlays/ViewportOverlay.h"

#include <Ember/Core/Time.h>

namespace Ember::UI {

	class StatsOverlay final : public ViewportOverlay
	{
	public:
		static Vector2f Draw(EditorLayer* editor, const ViewportOverlayPlacement& placement, TimeStep delta);

	protected:
		const char* GetWindowID() const override { return "Stats Overlay"; }
		void OnImGuiRender(EditorLayer* editor) override;

	private:
		StatsOverlay(TimeStep delta) : m_Delta(delta) {}

		TimeStep m_Delta;
	};

}
