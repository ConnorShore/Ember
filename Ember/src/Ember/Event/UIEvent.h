#pragma once

#include "Event.h"

#include <string>

namespace Ember {

	class UINotificationEvent : public Event
	{
	public:
		enum Severity
		{
			Info,
			Warning,
			Error
		};

	public:
		UINotificationEvent(const std::string& message, Severity severity = Severity::Info)
			: m_Message(message), m_Severity(severity) { }

		const std::string& GetMessage() const { return m_Message; }
		const Severity GetSeverity() const { return m_Severity; }

		EB_EVENT_TYPE_INITIALIZER(UINotification);
		EB_EVENT_CATEGORY_INITIALIZER(EventCategoryUI);

	private:
		std::string m_Message;
		Severity m_Severity = Severity::Info;
	};
}