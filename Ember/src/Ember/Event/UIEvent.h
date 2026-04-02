#pragma once

#include "Event.h"

#include <string>

namespace Ember {

	class UINotificationEvent : public Event
	{
	public:
		UINotificationEvent(const std::string& message)
			: m_Message(message) {
		}

		const std::string& GetMessage() const { return m_Message; }

		EB_EVENT_TYPE_INITIALIZER(UINotification);
		EB_EVENT_CATEGORY_INITIALIZER(EventCategoryUI);

	private:
		std::string m_Message;
	};
}