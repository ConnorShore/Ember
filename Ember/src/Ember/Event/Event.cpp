#include "ebpch.h"
#include "Event.h"

namespace Ember {

	//--------------------------------------------------------------------------------
	//------------------------------- Window Events ----------------------------------
	//--------------------------------------------------------------------------------
	class WindowCloseEvent : public Event
	{
	public:
		EventType GetEventType() const override { return EventType::WindowClose; }
		int GetCategoryFlags() const override { return EventCategory::EventCategoryWindow; }
		const char* GetName() const override { return "WindowCloseEvent"; }
	};

}