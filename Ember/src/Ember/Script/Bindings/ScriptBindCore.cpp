#include "ebpch.h"
#include "ScriptBindCore.h"
#include "Ember/Core/Application.h"
#include "Ember/ECS/System/RenderSystem.h"
#include "Ember/Script/ScriptEngine.h"

namespace Ember {

	void BindCore(sol::state& state)
	{
		state.new_usertype<UUID>("UUID",
			sol::constructors<UUID(), UUID(uint64_t)>(),
			sol::meta_function::equal_to, static_cast<bool(UUID::*)(const UUID&) const>(&UUID::operator==),
			sol::meta_function::to_string, [](const UUID& a) { return std::to_string((uint64_t)a); }
		);

		state.new_usertype<TimeStep>("TimeStep",
			sol::constructors<TimeStep(), TimeStep(float)>(),
			"Seconds", &TimeStep::Seconds,
			"Milliseconds", &TimeStep::Milliseconds,
			sol::meta_function::addition, [](const TimeStep& a, const TimeStep& b) {
				return TimeStep(a.Seconds() + b.Seconds());
			},
			sol::meta_function::equal_to, &TimeStep::operator==,
			sol::meta_function::to_string, [](const TimeStep& ts) {
				return std::to_string(ts.Seconds()) + "s";
			}
		);

		auto application = state.create_table("Time");
		application.set_function("Now", []() {
			return Timer::Now();
		});

		auto timer = state.create_table("Timer");
		timer.set_function("SetTimeout", [](sol::protected_function callback, float delaySeconds) {
			ScriptEngine::SetTimeout(std::move(callback), delaySeconds);
		});

		auto log = state.create_table("Log");
		log.set_function("Info", [](const std::string& message) {
			EB_CORE_INFO("{}", message);
		});

		log.set_function("Warn", [](const std::string& message) {
			EB_CORE_WARN("{}", message);
		});

		log.set_function("Error", [](const std::string& message) {
			EB_CORE_ERROR("{}", message);
		});

		auto window = state.create_table("Window");
		window.set_function("GetWidth", []() {
			return Application::Instance().GetWindow().GetWidth();
		});
		window.set_function("GetHeight", []() {
			return Application::Instance().GetWindow().GetHeight();
		});

		auto renderer = state.create_table("Renderer");
		renderer.set_function("GetViewportSize", []() {
			auto renderSystem = Application::Instance().GetSystem<RenderSystem>();
			return renderSystem->GetViewportSize();
		});
	}

}
