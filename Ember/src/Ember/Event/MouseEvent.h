#pragma once

#include "Event.h"
#include "Ember/Input/MouseCode.h"

namespace Ember {

	//////////////////////////////////////////////////////////////////////////
	// Mouse Pressed Event
	//////////////////////////////////////////////////////////////////////////
	class MousePressedEvent : public Event
	{
	public:
		MousePressedEvent(MouseButton button)
			: m_Button(button) { }

		const MouseButton GetMouseButton() const { return m_Button; }

		EB_EVENT_TYPE_INITIALIZER(MouseButtonPressed);
		EB_EVENT_CATEGORY_INITIALIZER(EventCategoryInput | EventCategoryMouse | EventCategoryMouseButton);

	private:
		MouseButton m_Button;
	};

	//////////////////////////////////////////////////////////////////////////
	// Mouse Released Event
	//////////////////////////////////////////////////////////////////////////
	class MouseReleasedEvent : public Event
	{
	public:
		MouseReleasedEvent(MouseButton button)
			: m_Button(button) {
		}

		const MouseButton GetMouseButton() const { return m_Button; }

		EB_EVENT_TYPE_INITIALIZER(MouseButtonReleased);
		EB_EVENT_CATEGORY_INITIALIZER(EventCategoryInput | EventCategoryMouse | EventCategoryMouseButton);

	private:
		MouseButton m_Button;
	};
}