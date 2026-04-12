#pragma once
#include <string>

namespace Ember {

	class ProjectSettingsDialog
	{
	public:
		enum class Category {
			General,
			Physics
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

		std::string m_PopupName = "Project Settings";
		Category m_SelectedCategory = Category::Physics; // Default tab
	};

}