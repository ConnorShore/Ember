#pragma once

#include "Panel.h"

namespace Ember {

	class NotificationPanel : public Panel
	{
	public:
		NotificationPanel(EditorContext* context) 
			: Panel("Notifications", context) {}
		virtual ~NotificationPanel() = default;

		void OnEvent(Event& event) override;
		void OnUpdate(TimeStep delta) override;
		void OnImGuiRender() override;

	private:
		bool OnNotification(UINotificationEvent& e);

	private:
		const float m_DisplayDuration = 3.0f;

	private:
		std::queue<std::string> m_NotificationQueue;
		std::string m_CurrentNotification;

		float m_CurrentTimer = 0.0f;
	};

}