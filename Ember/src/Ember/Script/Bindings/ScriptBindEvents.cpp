#include "ebpch.h"
#include "ScriptBindEvents.h"
#include "Ember/Script/ScriptEngine.h"

namespace Ember {

	void BindEvents(sol::state& state)
	{
		auto eventManager = state.create_table("EventManager");

		eventManager.set_function("Subscribe", [](const std::string& eventName, sol::protected_function callback) {
			ScriptEngine::SubscribeEvent(eventName, std::move(callback));
			});

		eventManager.set_function("Broadcast", [](const std::string& eventName, sol::variadic_args args) {
			ScriptEngine::BroadcastEvent(eventName, args);
			});
	}

}
