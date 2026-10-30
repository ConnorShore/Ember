#include "efpch.h"

#include "StatsOverlay.h"
#include "EditorLayer.h"

namespace Ember::UI {

	// Averages frame count over 1-second intervals for a stable FPS readout
	static float CalculateFPS(TimeStep delta)
	{
		static float fpsTimer = 0.0f;
		static float fps = 0.0f;
		static int frameCount = 0;

		fpsTimer += delta.Seconds();
		frameCount++;

		if (fpsTimer >= 1.0f)
		{
			fps = (float)frameCount / fpsTimer;

			fpsTimer = 0.0f;
			frameCount = 0;
		}

		return fps;
	}

	Vector2f StatsOverlay::Draw(EditorLayer* editor, const ViewportOverlayPlacement& placement, TimeStep delta)
	{
		StatsOverlay overlay(delta);
		return overlay.Render(editor, placement);
	}

	void StatsOverlay::OnImGuiRender(EditorLayer* editor)
	{
		ImGui::Text("Renderer Stats");
		ImGui::Separator();
		ImGui::Text("FPS: %.1f", CalculateFPS(m_Delta));

		if (auto activeScene = editor->GetContext().ActiveScene())
			ImGui::Text("Entities: %d", activeScene->GetAllEntities().size());
		else
			ImGui::Text("Entities: 0");
	}

}
