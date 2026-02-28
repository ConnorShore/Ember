#pragma once

#include "Event.h"

namespace Ember {

	//--------------------------------------------------------------------------------
	//----------------------------- Window Close Events ------------------------------
	//--------------------------------------------------------------------------------

	class WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() = default;

		EB_EVENT_TYPE_INITIALIZER(WindowClose);
		EB_EVENT_CATEGORY_INITIALIZER(EventCategoryApplication);
	};

}