#include "efpch.h"
#include "NotificationPanel.h"

namespace Ember {

	void NotificationPanel::OnEvent(Event& event)
	{
		EB_CREATE_DISPATCHER(event);
		EB_DISPATCH_EVENT(UINotificationEvent, OnNotification);
	}

	void NotificationPanel::OnUpdate(TimeStep delta)
	{
		if (m_CurrentNotification.Message.empty() && !m_NotificationQueue.empty())
		{
			m_CurrentNotification = m_NotificationQueue.front();
			m_NotificationQueue.pop();
			m_CurrentTimer = m_DisplayDuration;
		}
		else if (!m_CurrentNotification.Message.empty())
		{
			m_CurrentTimer -= delta;
			if (m_CurrentTimer <= 0.0f)
			{
				m_CurrentNotification.Message.clear();
			}
		}
	}

	void NotificationPanel::OnImGuiRender()
	{
		if (m_CurrentNotification.Message.empty())
			return;

		// Draw status bar at the bottom of the main viewport
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		float statusBarHeight = 24.0f;

		ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - statusBarHeight));
		ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, statusBarHeight));

		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoScrollWithMouse;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 4.0f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f)); // Dark background

		if (ImGui::Begin("##StatusBar", nullptr, windowFlags))
		{
			float alpha = std::clamp(m_CurrentTimer / 0.5f, 0.0f, 1.0f); // Fades out in the last 0.5 seconds

			switch (m_CurrentNotification.MessageSeverity)
			{
				case Severity::Info:
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, alpha)); // Light gray
					break;
				case Severity::Warning:
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.2f, alpha)); // Orange
					break;
				case Severity::Error:
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, alpha)); // Red
					break;
			}

			ImGui::Text("%s", m_CurrentNotification.Message.c_str());

			ImGui::PopStyleColor();
		}
		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
	}

	bool NotificationPanel::OnNotification(UINotificationEvent& e)
	{
		m_NotificationQueue.push({e.GetMessage(), e.GetSeverity()});

		// Log event
		switch (e.GetSeverity())
		{
			case Severity::Info:
				EB_CORE_INFO("UI Notification: {0}", e.GetMessage());
				break;
			case Severity::Warning:
				EB_CORE_WARN("UI Notification: {0}", e.GetMessage());
				break;
			case Severity::Error:
				EB_CORE_ERROR("UI Notification: {0}", e.GetMessage());
				break;
		}

		return false;
	}

}