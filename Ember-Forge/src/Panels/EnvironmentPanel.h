#pragma once

#include "Panel.h"

namespace Ember {

	class ColorGradePass;
	class ToneMapPass;

	class EnvironmentPanel : public Panel
	{
	public:
		EnvironmentPanel(EditorContext* context);
		virtual ~EnvironmentPanel() = default;

		void OnImGuiRender() override;

	private:
		void RenderSkyboxSettings();
		void RenderBloomSettings();
		void RenderFogSettings();
		void RenderFXAASettings();
		void RenderVignetteSettings();
		void RenderColorGradeLUTSettings(const SharedPtr<ColorGradePass>& colorGradePass, const SharedPtr<ToneMapPass>& toneMapPass);
		void RenderColorGradeSettings();
	};
}