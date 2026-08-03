#include "ebpch.h"
#include "ScriptBindCore.h"
#include "Ember/Core/Application.h"
#include "Ember/ECS/System/RenderSystem.h"
#include "Ember/Script/ScriptEngine.h"
#include "Ember/Script/ScriptProperty.h"

namespace Ember {

	static sol::table CreateScriptReferenceDefinition(sol::this_state state, ScriptReferenceKind kind, uint64_t value)
	{
		sol::state_view lua(state);
		sol::table ref = lua.create_table();
		ref["__ember_property_type"] = "Reference";
		ref["Kind"] = ScriptReferenceKindToString(kind);
		ref["Value"] = value;
		return ref;
	}

	// An empty Lua table carries no type, so array properties need an explicit tag to be classified.
	static sol::table CreateScriptReferenceArrayDefinition(sol::this_state state, ScriptReferenceKind kind)
	{
		sol::state_view lua(state);
		sol::table ref = lua.create_table();
		ref["__ember_property_type"] = "ReferenceArray";
		ref["Kind"] = ScriptReferenceKindToString(kind);
		return ref;
	}

	static uint64_t ToRawUUID(const UUID& uuid)
	{
		return (uint64_t)uuid;
	}

	void BindCore(sol::state& state)
	{
		state.new_usertype<UUID>("UUID",
			sol::constructors<UUID(), UUID(uint64_t)>(),
			"IsValid", [](const UUID& id) { return id != Constants::InvalidUUID; },
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
		log.set_function("Trace", [](const std::string& message) {
			EB_TRACE("{}", message);
			});
		log.set_function("Info", [](const std::string& message) {
			EB_INFO("{}", message);
			});
		log.set_function("Warn", [](const std::string& message) {
			EB_WARN("{}", message);
			});
		log.set_function("Error", [](const std::string& message) {
			EB_ERROR("{}", message);
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

		state.set_function("EntityRef", sol::overload(
			[](sol::this_state state) { return CreateScriptReferenceDefinition(state, ScriptReferenceKind::Entity, (uint64_t)Constants::InvalidUUID); },
			[](sol::this_state state, uint64_t uuid) { return CreateScriptReferenceDefinition(state, ScriptReferenceKind::Entity, uuid); },
			[](sol::this_state state, const UUID& uuid) { return CreateScriptReferenceDefinition(state, ScriptReferenceKind::Entity, ToRawUUID(uuid)); }
		));

		state.set_function("AssetRef", sol::overload(
			[](sol::this_state state, const std::string& kind) { return CreateScriptReferenceDefinition(state, ScriptReferenceKindFromString(kind), (uint64_t)Constants::InvalidUUID); },
			[](sol::this_state state, const std::string& kind, uint64_t uuid) { return CreateScriptReferenceDefinition(state, ScriptReferenceKindFromString(kind), uuid); },
			[](sol::this_state state, const std::string& kind, const UUID& uuid) { return CreateScriptReferenceDefinition(state, ScriptReferenceKindFromString(kind), ToRawUUID(uuid)); }
		));

		state.set_function("PrefabRef", sol::overload(
			[](sol::this_state state) { return CreateScriptReferenceDefinition(state, ScriptReferenceKind::Prefab, (uint64_t)Constants::InvalidUUID); },
			[](sol::this_state state, uint64_t uuid) { return CreateScriptReferenceDefinition(state, ScriptReferenceKind::Prefab, uuid); },
			[](sol::this_state state, const UUID& uuid) { return CreateScriptReferenceDefinition(state, ScriptReferenceKind::Prefab, ToRawUUID(uuid)); }
		));

		state.set_function("AudioClipRef", sol::overload(
			[](sol::this_state state) { return CreateScriptReferenceDefinition(state, ScriptReferenceKind::AudioClip, (uint64_t)Constants::InvalidUUID); },
			[](sol::this_state state, uint64_t uuid) { return CreateScriptReferenceDefinition(state, ScriptReferenceKind::AudioClip, uuid); },
			[](sol::this_state state, const UUID& uuid) { return CreateScriptReferenceDefinition(state, ScriptReferenceKind::AudioClip, ToRawUUID(uuid)); }
		));

		// Editor-populated reference lists; these reach scripts as 1-based Lua tables of UUIDs.
		state.set_function("EntityRefArray", [](sol::this_state state) {
			return CreateScriptReferenceArrayDefinition(state, ScriptReferenceKind::Entity);
			});

		state.set_function("AssetRefArray", [](sol::this_state state, const std::string& kind) {
			return CreateScriptReferenceArrayDefinition(state, ScriptReferenceKindFromString(kind));
			});

		state.set_function("PrefabRefArray", [](sol::this_state state) {
			return CreateScriptReferenceArrayDefinition(state, ScriptReferenceKind::Prefab);
			});

		state.set_function("AudioClipRefArray", [](sol::this_state state) {
			return CreateScriptReferenceArrayDefinition(state, ScriptReferenceKind::AudioClip);
			});
	}

}
