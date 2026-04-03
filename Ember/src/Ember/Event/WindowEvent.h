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
		WindowResizeEvent(uint32_t width, uint32_t height)
			: m_Width(width), m_Height(height) { }

		EB_EVENT_TYPE_INITIALIZER(WindowResize);
		EB_EVENT_CATEGORY_INITIALIZER(EventCategoryApplication);

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

	private:
		uint32_t m_Width, m_Height;
	};
}