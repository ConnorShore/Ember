#include "ebpch.h"
#include "ScriptBindCore.h"

namespace Ember {

	void BindCore(sol::state& state)
	{
		state.new_usertype<UUID>("UUID",
			sol::constructors<UUID(), UUID(uint64_t)>(),
			sol::meta_function::equal_to, static_cast<bool(UUID::*)(const UUID&) const>(&UUID::operator==),
			sol::meta_function::to_string, [](const UUID& a) { return std::to_string((uint64_t)a); }
		);

		state.set_function("Log", [](const std::string& message) {
			EB_CORE_INFO("{}", message);
		});

		state.set_function("LogWarn", [](const std::string& message) {
			EB_CORE_WARN("{}", message);
		});

		state.set_function("LogError", [](const std::string& message) {
			EB_CORE_ERROR("{}", message);
		});
	}

}
