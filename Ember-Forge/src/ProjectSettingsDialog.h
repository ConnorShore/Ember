#pragma once
#include <string>

namespace Ember {

	class ProjectSettingsDialog
	{
	public:
		enum class Category {
			General,
			Input,
			Physics,
			Rendering
		};

		ProjectSettingsDialog();
		~ProjectSettingsDialog();

		void OnImGuiRender();

		std::string GetPopupName() const { return m_PopupName; }

	private:
		void RenderLeftPane();
		void RenderRightPane();

		void RenderGeneralSettings();
		void RenderPhysicsSettings();
		void RenderRenderingSettings();
		void RenderInputSettings();

		std::string m_PopupName = "Project Settings";
		Category m_SelectedCategory = Category::General; // Default tab
	};

}