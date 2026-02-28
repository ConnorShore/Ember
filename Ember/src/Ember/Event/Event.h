#pragma once 

namespace Ember {
	
	enum class EventType
	{
		None = 0,
		WindowClose,
		WindowResize,
		WindowFocus,
		WindowLostFocus,
		WindowMoved,
		KeyPressed,
		KeyReleased,
		MouseButtonPressed,
		MouseButtonReleased,
		MouseMoved,
		MouseScrolled
	};

	enum EventCategory
	{
		None = 0,
		EventCategoryApplication =	1 << 0,
		EventCategoryWindow =		1 << 1,
		EventCategoryInput =		1 << 2,
		EventCategoryKeyboard =		1 << 3,
		EventCategoryMouse =		1 << 4,
		EventCategoryMouseButton =	1 << 5
	};

	class Event
	{
	public:
		Event() = default;
		virtual ~Event() = default;
		virtual EventType GetEventType() const = 0;
		virtual int GetCategoryFlags() const = 0;
		virtual const char* GetName() const = 0;

		bool IsInCategory(EventCategory category) { return GetCategoryFlags() & category; }
		bool Handled() const { return m_Handled; }
		void Consume() { m_Handled = true; }

	private:
		bool m_Handled = false;
	};

	class EventDispatcher
	{
	public:
		EventDispatcher(Event& event) : m_Event(event) {}

		template<typename T, typename F>
		bool Dispatch(const F& func)
		{
			if (m_Event.GetEventType() == T::GetStaticType())
			{
				if (func(static_cast<T&>(m_Event)))
				{
					m_Event.Consume();
				}

				return true;
			}
			return false;
		}

	private:
		Event& m_Event;
	};
}