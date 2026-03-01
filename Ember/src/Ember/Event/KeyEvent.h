#pragma once

#include "Event.h"

#include "Ember/Input/KeyCode.h"

namespace Ember {

#define EB_KEY_EVENT_CATEGORY EventCategoryInput | EventCategoryKeyboard

	class KeyPressedEvent : public Event
	{
	public:
		KeyPressedEvent(KeyCode key)
			: m_Key(key) { }

		const KeyCode GetKeyCode() const { return m_Key; }

		EB_EVENT_TYPE_INITIALIZER(KeyPressed);
		EB_EVENT_CATEGORY_INITIALIZER(EB_KEY_EVENT_CATEGORY);

	private:
		KeyCode m_Key;
	};

	class KeyRepeatEvent : public Event
	{
	public:
		KeyRepeatEvent(KeyCode key)
			: m_Key(key) { }

		const KeyCode GetKeyCode() const { return m_Key; }

		EB_EVENT_TYPE_INITIALIZER(KeyRepeat);
		EB_EVENT_CATEGORY_INITIALIZER(EB_KEY_EVENT_CATEGORY);

	private:
		KeyCode m_Key;
	};

	class KeyReleasedEvent : public Event
	{
		public:
			KeyReleasedEvent(KeyCode key)
				: m_Key(key) { }

			const KeyCode GetKeyCode() const { return m_Key; }

			EB_EVENT_TYPE_INITIALIZER(KeyReleased);
			EB_EVENT_CATEGORY_INITIALIZER(EB_KEY_EVENT_CATEGORY);

	private:
		KeyCode m_Key;
	};

}