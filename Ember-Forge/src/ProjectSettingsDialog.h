#pragma once

#include <Ember/Core/Application.h>
#include <Ember/Input/InputTrigger.h>

#include <string>

namespace Ember {

	struct InputAction;

	class ProjectSettingsDialog
	{
	public:
		enum class Category {
			General,
			Input,
			Physics,
			Rendering
		};

		ProjectSettingsDialog() = default;
		~ProjectSettingsDialog() = default;

		void OnImGuiRender();

		std::string GetPopupName() const { return m_PopupName; }

	private:
		void RenderLeftPane();
		void RenderRightPane();

		void RenderGeneralSettings();
		void RenderPhysicsSettings();
		void RenderRenderingSettings();
		void RenderInputSettings();

		// Input action list
		void RenderAddActionRow();
		void RenderActionRow(int actionIndex, const InputAction& action);
		void RenderTriggerRow(int actionIndex, int triggerIndex, const InputTrigger& trigger);

		// Called from OnImGuiRender so the nested modals are never parented to a child window
		void RenderTriggerConfigPopup();
		void RenderRemoveActionPopup();

		// Queues the trigger popup; a triggerIndex of -1 means a new trigger rather than an edit
		void OpenTriggerConfigPopup(int actionIndex, int triggerIndex);

		// One device section of the trigger picker; returns true when a control was double-clicked
		bool RenderKeyboardSection();
		bool RenderMouseSection();

		// The only four places the input UI touches the backend - everything else just reads
		void AddInputAction(const std::string& name);
		void RemoveInputAction(int index);
		void CommitPendingTrigger();
		void RemoveInputTrigger(int actionIndex, int triggerIndex);

	private:
		std::string m_PopupName = "Project Settings";
		Category m_SelectedCategory = Category::General; // Default tab

		// Input action authoring state
		char m_NewActionName[64] = "";
		bool m_FocusNewActionField = false;

		// Action queued for the remove confirmation, and the request that opens that popup
		int m_ActionPendingRemoval = -1;
		bool m_RemovePopupRequested = false;

		InputActionManager& m_InputActionManager = Application::Instance().GetInputActionManager();

		// Trigger the config popup is building, and where it gets written back
		InputTrigger m_PendingTrigger;
		int m_TriggerActionIndex = -1;
		int m_TriggerEditIndex = -1;    // -1 while adding rather than editing an existing trigger
		char m_TriggerSearch[64] = "";
		bool m_TriggerPopupRequested = false;
	};

}
