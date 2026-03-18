#pragma once

#include "Event.h"
#include "Ember/Input/MouseCode.h"
#include "Ember/Math/Math.h"

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

	//////////////////////////////////////////////////////////////////////////
	// Mouse Moved Event
	//////////////////////////////////////////////////////////////////////////
	
	class MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent(const Vector2f& position)
			: m_Position(position) {
		}

		const Vector2f& GetMousePosition() const { return m_Position; }

		EB_EVENT_TYPE_INITIALIZER(MouseMoved);
		EB_EVENT_CATEGORY_INITIALIZER(EventCategoryInput | EventCategoryMouse);

	private:
		Vector2f m_Position;
	};

	//////////////////////////////////////////////////////////////////////////
	// Mouse Scrolled Event
	//////////////////////////////////////////////////////////////////////////

	class MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent(const Vector2f& offset)
			: m_Offset(offset) {
		}

		float GetXOffset() const { return m_Offset.x; }
		float GetYOffset() const { return m_Offset.y; }
		const Vector2f& GetScrollOffset() const { return m_Offset; }

		EB_EVENT_TYPE_INITIALIZER(MouseScrolled);
		EB_EVENT_CATEGORY_INITIALIZER(EventCategoryInput | EventCategoryMouse);

	private:
		Vector2f m_Offset;
	};
}