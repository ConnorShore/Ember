#pragma once

#include "Panel.h"

namespace Ember {

	class EnvironmentPanel : public Panel
	{
	public:
		EnvironmentPanel(EditorContext* context);
		virtual ~EnvironmentPanel() = default;

		void OnImGuiRender() override;

	private:
		void RenderSkyboxSettings();
		void RenderBloomSettings();
		void RenderFXAASettings();
		void RenderColorGradeSettings();
	};
}