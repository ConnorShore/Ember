#pragma once 

namespace Ember {

#define EB_EVENT_FUNCTION(f) [this](Event& e) { f; }
#define EB_CREATE_DISPATCHER(event) EventDispatcher dispatcher(event);
#define EB_DISPATCH_EVENT(eventType, handler) dispatcher.Dispatch<eventType>([this](eventType e) { return handler(e); });


#define EB_EVENT_TYPE_INITIALIZER(type) static EventType GetStaticType() { return EventType::type; }\
								virtual EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetName() const override { return #type; }
#define EB_EVENT_CATEGORY_INITIALIZER(category) virtual int GetCategoryFlags() const override { return category; }

	
	enum class EventType
	{
		None = 0,
		WindowClose,
		WindowResize,
		WindowFocus,
		WindowLostFocus,
		WindowMoved,
		KeyPressed,
		KeyRepeat,
		KeyReleased,
		MouseButtonPressed,
		MouseButtonReleased,
		MouseMoved,
		MouseScrolled
		// TODO: Mouse enter/exist, focus, etc.
	};

	enum EventCategory
	{
		None = 0,
		EventCategoryApplication =	1 << 0,
		EventCategoryInput =		1 << 1,
		EventCategoryKeyboard =		1 << 2,
		EventCategoryMouse =		1 << 3,
		EventCategoryMouseButton =	1 << 4
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

	private:
		friend class EventDispatcher;
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
					m_Event.m_Handled = true;
				}

				return true;
			}
			return false;
		}

	private:
		Event& m_Event;
	};
}