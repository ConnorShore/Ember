#include "efpch.h"
#include "WelcomeDialog.h"

#include "Utils/RecentProjectSerializer.h"

#include <Ember/Core/Application.h>
#include <Ember/Core/Paths.h>
#include <Ember/Render/Texture2D.h>

#include <cmath>

namespace Ember {

	namespace {

		const ImVec4 c_Accent = ImVec4(0.88f, 0.40f, 0.10f, 1.00f);
		const ImVec4 c_AccentHovered = ImVec4(0.95f, 0.47f, 0.15f, 1.00f);
		const ImVec4 c_AccentActive = ImVec4(0.80f, 0.35f, 0.08f, 1.00f);
		const ImVec4 c_TextBright = ImVec4(0.94f, 0.93f, 0.92f, 1.00f);
		const ImVec4 c_TextMuted = ImVec4(0.55f, 0.54f, 0.53f, 1.00f);

		const ImVec4 c_PanelBg = ImVec4(0.105f, 0.101f, 0.098f, 1.00f);
		const ImVec4 c_PanelBorder = ImVec4(0.22f, 0.20f, 0.19f, 1.00f);
		const ImVec4 c_HeaderBand = ImVec4(0.140f, 0.133f, 0.127f, 1.00f);
		const ImVec4 c_ListBg = ImVec4(0.075f, 0.072f, 0.070f, 1.00f);

		const float c_PanelRounding = 10.0f;
		const float c_HeaderHeight = 132.0f;
		const float c_BodyPadding = 26.0f;
		const float c_SectionLabelSize = 12.0f;

		// Fakes a radial gradient by stacking translucent circles - cheap and good enough for a backdrop.
		void DrawRadialGlow(ImDrawList* drawList, const ImVec2& center, float radius, const ImVec4& color, float peakAlpha)
		{
			const int steps = 24;

			ImVec4 layerColor = color;
			layerColor.w = peakAlpha / (float)steps;

			const ImU32 layerColorU32 = ImGui::ColorConvertFloat4ToU32(layerColor);
			for (int i = steps; i > 0; --i)
				drawList->AddCircleFilled(center, radius * ((float)i / (float)steps), layerColorU32, 64);
		}

		void SectionLabel(const char* label)
		{
			ImGui::PushFont(nullptr, c_SectionLabelSize);
			ImGui::PushStyleColor(ImGuiCol_Text, c_TextMuted);
			ImGui::TextUnformatted(label);
			ImGui::PopStyleColor();
			ImGui::PopFont();
		}

	}

	WelcomeDialog::WelcomeDialog()
	{
		// Roams with the user profile, and survives both an editor update and an uninstall.
		m_RecentProjectsFilePath = Paths::UserConfigDir() / c_RecentProjectsFileName;
	}

	void WelcomeDialog::OnAttach()
	{
		auto icon = Application::Instance().GetAssetManager().Load<Texture2D>((Paths::EditorAssets() / "images/EmberIcon.png").string());
		if (icon)
			m_IconTextureID = (ImTextureID)(intptr_t)icon->GetID();

		// Load recent projects
		RecentProjectSerializer serializer(m_RecentProjects);
		if (!serializer.Deserialize(m_RecentProjectsFilePath))
			EB_CORE_WARN("Failed to load recent projects from file: {0}", m_RecentProjectsFilePath.string());
	}

	void WelcomeDialog::AddRecentProject(const RecentProject& project)
	{
		if (project.Path.empty())
			return;

		// Re-adding an existing entry moves it back to the top rather than duplicating it
		RemoveRecentProject(project.Path);
		m_RecentProjects.insert(m_RecentProjects.begin(), project);
	}

	void WelcomeDialog::RemoveRecentProject(const std::string& path)
	{
		std::erase_if(m_RecentProjects, [&path](const RecentProject& project) { return project.Path == path; });
	}

	void WelcomeDialog::RemoveOldestRecentProject()
	{
		m_RecentProjects.pop_back();
	}

	void WelcomeDialog::OnImGuiRender(bool showPanel /* = true */)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImVec2 panelSize = ImVec2(
			std::min(940.0f, std::max(520.0f, viewport->Size.x - 100.0f)),
			std::min(600.0f, std::max(380.0f, viewport->Size.y - 100.0f)));

		ImVec2 panelPos = ImVec2(
			viewport->Pos.x + (viewport->Size.x - panelSize.x) * 0.5f,
			viewport->Pos.y + (viewport->Size.y - panelSize.y) * 0.5f);

		RenderBackdrop(panelPos, panelSize);

		if (!showPanel)
			return;

		ImGui::SetNextWindowPos(panelPos);
		ImGui::SetNextWindowSize(panelSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, c_PanelRounding);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, c_PanelBg);
		ImGui::PushStyleColor(ImGuiCol_Border, c_PanelBorder);

		if (ImGui::Begin("##WelcomePanel", nullptr, flags))
		{
			float headerBottom = RenderHeader(panelSize.x);

			ImVec2 bodyPos = ImVec2(ImGui::GetWindowPos().x + c_BodyPadding, headerBottom + c_BodyPadding);
			ImGui::SetCursorScreenPos(bodyPos);

			const float columnGap = 24.0f;
			float bodyWidth = panelSize.x - c_BodyPadding * 2.0f;
			float bodyHeight = (ImGui::GetWindowPos().y + panelSize.y - c_BodyPadding) - bodyPos.y;
			float recentWidth = std::floor((bodyWidth - columnGap) * 0.58f);
			float actionsWidth = bodyWidth - columnGap - recentWidth;

			ImGui::BeginGroup();
			RenderRecentProjects(ImVec2(recentWidth, bodyHeight));
			ImGui::EndGroup();

			ImGui::SameLine(0.0f, columnGap);

			ImGui::BeginGroup();
			RenderActions(ImVec2(actionsWidth, bodyHeight));
			ImGui::EndGroup();
		}
		ImGui::End();

		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(3);

		// If an error occurred while opening a project, show a popup with the message
		if (m_ShowErrorPopup)
		{
			ImGui::OpenPopup("Error");
			m_ShowErrorPopup = false;
		}

		RenderErrorPopup();
	}

	void WelcomeDialog::RenderBackdrop(const ImVec2& panelPos, const ImVec2& panelSize)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImDrawList* drawList = ImGui::GetBackgroundDrawList(viewport);

		ImVec2 backdropMin = viewport->Pos;
		ImVec2 backdropMax = ImVec2(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y);

		const ImU32 topColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.086f, 0.080f, 0.075f, 1.00f));
		const ImU32 bottomColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.028f, 0.027f, 0.030f, 1.00f));
		drawList->AddRectFilledMultiColor(backdropMin, backdropMax, topColor, topColor, bottomColor, bottomColor);

		// Warm glow behind the card so it reads as lit from within rather than sitting on flat grey
		ImVec2 glowCenter = ImVec2(panelPos.x + panelSize.x * 0.5f, panelPos.y + panelSize.y * 0.32f);
		float glowRadius = std::max(panelSize.x, panelSize.y) * 0.95f;
		DrawRadialGlow(drawList, glowCenter, glowRadius, c_Accent, 0.20f);
	}

	float WelcomeDialog::RenderHeader(float panelWidth)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		ImVec2 headerMin = ImGui::GetCursorScreenPos();
		ImVec2 headerMax = ImVec2(headerMin.x + panelWidth, headerMin.y + c_HeaderHeight);

		drawList->AddRectFilled(headerMin, headerMax, ImGui::ColorConvertFloat4ToU32(c_HeaderBand), c_PanelRounding, ImDrawFlags_RoundCornersTop);
		drawList->AddLine(ImVec2(headerMin.x, headerMax.y), ImVec2(headerMax.x, headerMax.y), ImGui::ColorConvertFloat4ToU32(c_PanelBorder), 1.0f);

		// Ember accent underlining the header, fading out toward the right
		ImVec4 accentFade = c_Accent;
		accentFade.w = 0.0f;
		drawList->AddRectFilledMultiColor(
			ImVec2(headerMin.x, headerMax.y - 2.0f), ImVec2(headerMin.x + panelWidth * 0.5f, headerMax.y),
			ImGui::ColorConvertFloat4ToU32(c_Accent), ImGui::ColorConvertFloat4ToU32(accentFade),
			ImGui::ColorConvertFloat4ToU32(accentFade), ImGui::ColorConvertFloat4ToU32(c_Accent));

		const float iconSize = 88.0f;
		ImVec2 iconPos = ImVec2(headerMin.x + 34.0f, headerMin.y + (c_HeaderHeight - iconSize) * 0.5f);

		if (m_IconTextureID != ImTextureID_Invalid)
		{
			DrawRadialGlow(drawList, ImVec2(iconPos.x + iconSize * 0.5f, iconPos.y + iconSize * 0.5f), iconSize * 1.1f, c_Accent, 0.30f);

			ImGui::SetCursorScreenPos(iconPos);
			// Textures are loaded bottom-up, so the UVs are flipped to display the icon upright
			ImGui::Image(m_IconTextureID, ImVec2(iconSize, iconSize), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		}

		float textX = iconPos.x + iconSize + 26.0f;

		ImGui::SetCursorScreenPos(ImVec2(textX, headerMin.y + 32.0f));
		ImGui::PushFont(nullptr, 38.0f);
		ImGui::TextColored(c_Accent, "Ember");
		ImGui::SameLine(0.0f, 0.0f);
		ImGui::TextColored(c_TextBright, " Forge");
		ImGui::PopFont();

		ImGui::SetCursorScreenPos(ImVec2(textX, headerMin.y + 84.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, c_TextMuted);
		ImGui::TextUnformatted("Create a new project or open an existing one to get started.");
		ImGui::PopStyleColor();

		// Widgets above were placed by hand, so put the layout cursor back under the band
		ImGui::SetCursorScreenPos(ImVec2(headerMin.x, headerMax.y));
		return headerMax.y;
	}

	void WelcomeDialog::RenderRecentProjects(const ImVec2& size)
	{
		float startY = ImGui::GetCursorPosY();

		SectionLabel("RECENT PROJECTS");
		ImGui::Spacing();

		float listHeight = size.y - (ImGui::GetCursorPosY() - startY);

		ImGui::PushStyleColor(ImGuiCol_ChildBg, c_ListBg);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

		if (ImGui::BeginChild("RecentProjectsList", ImVec2(size.x, listHeight), ImGuiChildFlags_Borders))
		{
			if (m_RecentProjects.empty())
			{
				const char* emptyTitle = "No recent projects";
				const char* emptyHint = "Projects you open will be listed here.";

				ImVec2 start = ImGui::GetCursorPos();
				ImVec2 avail = ImGui::GetContentRegionAvail();
				float blockHeight = ImGui::GetTextLineHeight() * 2.0f + ImGui::GetStyle().ItemSpacing.y;

				ImGui::SetCursorPosY(start.y + std::max(0.0f, (avail.y - blockHeight) * 0.5f));

				ImGui::PushStyleColor(ImGuiCol_Text, c_TextMuted);
				ImGui::SetCursorPosX(start.x + (avail.x - ImGui::CalcTextSize(emptyTitle).x) * 0.5f);
				ImGui::TextUnformatted(emptyTitle);
				ImGui::SetCursorPosX(start.x + (avail.x - ImGui::CalcTextSize(emptyHint).x) * 0.5f);
				ImGui::TextUnformatted(emptyHint);
				ImGui::PopStyleColor();
			}
			else
			{
				for (size_t i = 0; i < m_RecentProjects.size(); i++)
				{
					RecentProjectAction action = RenderRecentProjectRow(m_RecentProjects[i], (int)i);
					if (action == RecentProjectAction::None)
						continue;

					// Copied because either action rewrites this list, invalidating the element it came from
					std::string projectPath = m_RecentProjects[i].Path;

					if (action == RecentProjectAction::Remove)
					{
						RemoveRecentProject(projectPath);
						SaveRecentProjectsToFile();
						break;
					}

					// Check if the project file still exists before trying to open it, otherwise show a warning dialog and remove it from the list
					if (!std::filesystem::exists(projectPath))
					{
						EB_CORE_WARN("Recent project file does not exist: {0}", projectPath);
						m_ShowErrorPopup = true;
						m_ErrorPopupMessage = "The project file does not exist:\n" + projectPath + "\n\nIt will be removed from the recent projects list.";
						RemoveRecentProject(projectPath);
						SaveRecentProjectsToFile();
						break;
					}

					if (m_OpenProjectCallback)
						m_OpenProjectCallback(projectPath);

					break;
				}
			}
		}
		ImGui::EndChild();

		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();
	}

	void WelcomeDialog::RenderActions(const ImVec2& size)
	{
		float startY = ImGui::GetCursorPosY();

		SectionLabel("GET STARTED");
		ImGui::Spacing();

		const float buttonHeight = 52.0f;

		ImGui::PushStyleColor(ImGuiCol_Button, c_Accent);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, c_AccentHovered);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, c_AccentActive);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		if (ImGui::Button("New Project", ImVec2(size.x, buttonHeight)) && m_NewProjectCallback)
			m_NewProjectCallback();
		ImGui::PopStyleColor(4);

		ImGui::Spacing();

		// An empty path tells the caller to browse for a project file
		if (ImGui::Button("Open Project", ImVec2(size.x, buttonHeight)) && m_OpenProjectCallback)
			m_OpenProjectCallback(std::string());

		const char* hint = "Open Project browses for an .ebproj file.";

		ImVec2 hintSize = ImGui::CalcTextSize(hint, nullptr, false, size.x);
		float remaining = size.y - (ImGui::GetCursorPosY() - startY);
		if (remaining > hintSize.y + 20.0f)
			ImGui::Dummy(ImVec2(1.0f, remaining - hintSize.y - 20.0f));

		// Drawn by hand rather than with Separator() so it stops at the column edge
		ImVec2 dividerPos = ImGui::GetCursorScreenPos();
		ImGui::GetWindowDrawList()->AddLine(dividerPos, ImVec2(dividerPos.x + size.x, dividerPos.y), ImGui::ColorConvertFloat4ToU32(c_PanelBorder), 1.0f);
		ImGui::Dummy(ImVec2(size.x, 1.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, c_TextMuted);
		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + size.x);
		ImGui::TextUnformatted(hint);
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();
	}

	WelcomeDialog::RecentProjectAction WelcomeDialog::RenderRecentProjectRow(const RecentProject& project, int index)
	{
		const float rowHeight = 54.0f;
		const float rowWidth = ImGui::GetContentRegionAvail().x;
		const float removeSize = 22.0f;

		ImVec2 rowMin = ImGui::GetCursorScreenPos();
		ImVec2 rowMax = ImVec2(rowMin.x + rowWidth, rowMin.y + rowHeight);
		ImVec2 removeMin = ImVec2(rowMax.x - removeSize - 10.0f, rowMin.y + (rowHeight - removeSize) * 0.5f);
		ImVec2 removeCenter = ImVec2(removeMin.x + removeSize * 0.5f, removeMin.y + removeSize * 0.5f);

		ImGui::PushID(index);

		// Submitted before the row so it owns hover/activation over its own rect, which stops the row underneath from firing too
		ImGui::SetCursorScreenPos(removeMin);
		bool removeClicked = ImGui::InvisibleButton("##RemoveRecentProject", ImVec2(removeSize, removeSize));
		bool removeHovered = ImGui::IsItemHovered();
		bool removeHeld = ImGui::IsItemActive();

		ImGui::SetCursorScreenPos(rowMin);
		bool clicked = ImGui::InvisibleButton("##RecentProjectRow", ImVec2(rowWidth, rowHeight));
		bool hovered = ImGui::IsItemHovered() || removeHovered;
		bool held = ImGui::IsItemActive();
		ImGui::PopID();

		if (hovered)
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		ImVec4 rowColor = held
			? ImVec4(0.20f, 0.18f, 0.17f, 1.00f)
			: (hovered ? ImVec4(0.165f, 0.155f, 0.148f, 1.00f) : ImVec4(0.118f, 0.113f, 0.110f, 1.00f));

		drawList->AddRectFilled(rowMin, rowMax, ImGui::ColorConvertFloat4ToU32(rowColor), 5.0f);
		if (hovered || held)
			drawList->AddRectFilled(rowMin, ImVec2(rowMin.x + 3.0f, rowMax.y), ImGui::ColorConvertFloat4ToU32(c_Accent), 2.0f);

		// Clipped short of the remove button so a long project path cannot run underneath it
		ImGui::PushClipRect(rowMin, ImVec2(removeMin.x - 8.0f, rowMax.y), true);

		ImGui::SetCursorScreenPos(ImVec2(rowMin.x + 14.0f, rowMin.y + 9.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, c_TextBright);
		ImGui::TextUnformatted(project.Name.c_str());
		ImGui::PopStyleColor();

		ImGui::SetCursorScreenPos(ImVec2(rowMin.x + 14.0f, rowMin.y + 11.0f + ImGui::GetTextLineHeight()));
		ImGui::PushFont(nullptr, 12.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, c_TextMuted);
		ImGui::TextUnformatted(project.Path.c_str());
		ImGui::PopStyleColor();
		ImGui::PopFont();

		ImGui::PopClipRect();

		if (removeHovered || removeHeld)
			drawList->AddCircleFilled(removeCenter, removeSize * 0.5f, ImGui::ColorConvertFloat4ToU32(removeHeld ? c_AccentActive : c_AccentHovered), 24);

		// Drawn rather than typed so the X needs no glyph in the font atlas
		const float arm = 4.5f;
		const ImU32 removeColor = ImGui::ColorConvertFloat4ToU32((removeHovered || removeHeld) ? c_TextBright : c_TextMuted);
		drawList->AddLine(ImVec2(removeCenter.x - arm, removeCenter.y - arm), ImVec2(removeCenter.x + arm, removeCenter.y + arm), removeColor, 1.6f);
		drawList->AddLine(ImVec2(removeCenter.x - arm, removeCenter.y + arm), ImVec2(removeCenter.x + arm, removeCenter.y - arm), removeColor, 1.6f);

		// The labels above were positioned by hand, so a zero-height item closes the row and advances the layout cursor
		ImGui::SetCursorScreenPos(ImVec2(rowMin.x, rowMax.y));
		ImGui::Dummy(ImVec2(rowWidth, 0.0f));

		if (removeClicked)
			return RecentProjectAction::Remove;

		return clicked ? RecentProjectAction::Open : RecentProjectAction::None;
	}

	void WelcomeDialog::SaveRecentProjectsToFile()
	{
		RecentProjectSerializer serializer(m_RecentProjects);
		if (!serializer.Serialize(m_RecentProjectsFilePath))
		{
			EB_CORE_ERROR("Failed to save recent projects to file: {}", m_RecentProjectsFilePath.string());
		}
	}

	void WelcomeDialog::RenderErrorPopup()
	{
		if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextUnformatted(m_ErrorPopupMessage.c_str());
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
			if (ImGui::Button("OK", ImVec2(120, 0)))
			{
				m_ErrorPopupMessage.clear();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}

	// Taken by value because AddRecentProject erases any existing entry, which would dangle a reference into the list
	void WelcomeDialog::UpdateRecentProjectsAndSave(RecentProject project)
	{
		AddRecentProject(project);
		if (m_RecentProjects.size() > c_MaxRecentProjects)
			RemoveOldestRecentProject();

		SaveRecentProjectsToFile();
	}

}
