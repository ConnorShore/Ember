#pragma once

#include "Utils/RecentProject.h"

#include <functional>
#include <string>
#include <vector>

namespace Ember {

	class WelcomeDialog
	{
	public:

		// Both are fire-and-forget requests - the editor may not finish (or may abandon) the action this frame,
		// so it reports the result back through UpdateRecentProjectsAndSave rather than through a return value.
		using NewProjectCallback = std::function<void()>;
		using OpenProjectCallback = std::function<void(const std::string& projectPath)>;

		WelcomeDialog();
		~WelcomeDialog() = default;

		void OnAttach();

		// showPanel lets the caller keep the backdrop while another dialog (e.g. New Project) owns the screen.
		void OnImGuiRender(bool showPanel = true);

		void SetNewProjectCallback(NewProjectCallback callback) { m_NewProjectCallback = std::move(callback); }
		void SetOpenProjectCallback(OpenProjectCallback callback) { m_OpenProjectCallback = std::move(callback); }

		// Recent entries are supplied by the caller - the dialog only displays what it is given.
		void AddRecentProject(const RecentProject& project);
		void RemoveRecentProject(const std::string& path);
		void RemoveOldestRecentProject();
		void ClearRecentProjects() { m_RecentProjects.clear(); }
		const std::vector<RecentProject>& GetRecentProjects() const { return m_RecentProjects; }

		// Called by the editor once a project has actually opened, which may be several frames after the request
		void UpdateRecentProjectsAndSave(RecentProject project);

	private:
		// A row carries two controls, so it reports which one the user hit rather than a bare clicked flag
		enum class RecentProjectAction { None, Open, Remove };

		void RenderBackdrop(const ImVec2& panelPos, const ImVec2& panelSize);
		float RenderHeader(float panelWidth);
		void RenderRecentProjects(const ImVec2& size);
		void RenderActions(const ImVec2& size);
		RecentProjectAction RenderRecentProjectRow(const RecentProject& project, int index);
		void SaveRecentProjectsToFile();
		void RenderErrorPopup();

	private:
		const uint8_t c_MaxRecentProjects = 5;
		const std::string c_RecentProjectsFileName = "recent_projects.yaml";

		std::vector<RecentProject> m_RecentProjects;
		std::filesystem::path m_RecentProjectsFilePath;

		NewProjectCallback m_NewProjectCallback;
		OpenProjectCallback m_OpenProjectCallback;

		ImTextureID m_IconTextureID = ImTextureID_Invalid;

		bool m_ShowErrorPopup = false;
		std::string m_ErrorPopupMessage;
	};

}
