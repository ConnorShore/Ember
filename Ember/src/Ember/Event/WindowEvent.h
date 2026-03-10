#pragma once

#include "Event.h"

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Window Close Event
	//////////////////////////////////////////////////////////////////////////
	class WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() = default;

		EB_EVENT_TYPE_INITIALIZER(WindowClose);
		EB_EVENT_CATEGORY_INITIALIZER(EventCategoryApplication);
	};

	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(unsigned int width, unsigned int height)
			: m_Width(width), m_Height(height) { }

		EB_EVENT_TYPE_INITIALIZER(WindowResize);
		EB_EVENT_CATEGORY_INITIALIZER(EventCategoryApplication);

		const unsigned int GetWidth() const { return m_Width; }
		const unsigned int GetHeight() const { return m_Height; }

	private:
		unsigned int m_Width, m_Height;
	};
}