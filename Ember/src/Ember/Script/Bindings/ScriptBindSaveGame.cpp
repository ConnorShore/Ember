#include "ebpch.h"
#include "ScriptBindSaveGame.h"

#include "Ember/Core/Application.h"
#include "Ember/Core/SaveGameManager.h"

namespace Ember {

	void BindSaveGame(sol::state& state)
	{
		// Keys are taken as string_view so a get/set from Lua reads straight out of the Lua string
		// instead of heap-allocating a std::string on every call.
		state.new_usertype<SaveFileHandle>("SaveFile",
			"SetInt", [](SaveFileHandle& self, std::string_view key, int value) { self.Resolve().SetInt(key, value); },
			"SetFloat", [](SaveFileHandle& self, std::string_view key, float value) { self.Resolve().SetFloat(key, value); },
			"SetBool", [](SaveFileHandle& self, std::string_view key, bool value) { self.Resolve().SetBool(key, value); },
			"SetString", [](SaveFileHandle& self, std::string_view key, std::string value) { self.Resolve().SetString(key, std::move(value)); },
			// sol2 does not honour C++ default arguments, so each arity is bound explicitly.
			"GetInt", sol::overload(
				[](SaveFileHandle& self, std::string_view key) { return self.Resolve().GetInt(key); },
				[](SaveFileHandle& self, std::string_view key, int defaultValue) { return self.Resolve().GetInt(key, defaultValue); }
			),
			"GetFloat", sol::overload(
				[](SaveFileHandle& self, std::string_view key) { return self.Resolve().GetFloat(key); },
				[](SaveFileHandle& self, std::string_view key, float defaultValue) { return self.Resolve().GetFloat(key, defaultValue); }
			),
			"GetBool", sol::overload(
				[](SaveFileHandle& self, std::string_view key) { return self.Resolve().GetBool(key); },
				[](SaveFileHandle& self, std::string_view key, bool defaultValue) { return self.Resolve().GetBool(key, defaultValue); }
			),
			"GetString", sol::overload(
				[](SaveFileHandle& self, std::string_view key) { return self.Resolve().GetString(key); },
				[](SaveFileHandle& self, std::string_view key, const std::string& defaultValue) { return self.Resolve().GetString(key, defaultValue); }
			),
			"Has", [](SaveFileHandle& self, std::string_view key) { return self.Resolve().Has(key); },
			"Remove", [](SaveFileHandle& self, std::string_view key) { return self.Resolve().Remove(key); },
			"Clear", [](SaveFileHandle& self) { self.Resolve().Clear(); },
			"Count", [](SaveFileHandle& self) { return (int)self.Resolve().Size(); },
			"GetName", [](SaveFileHandle& self) { return self.Resolve().GetName(); },
			"IsValid", &SaveFileHandle::IsValid,
			"Save", [](SaveFileHandle& self) { return self.Owner->Save(self.Resolve().GetName()); }
		);

		state.new_usertype<SaveGameManager>("SaveSystem",
			"Open", [](SaveGameManager& self, std::string_view name) { return self.Open(name); },
			"Reload", [](SaveGameManager& self, std::string_view name) { return self.Reload(name); },
			"SaveAll", [](SaveGameManager& self) { return self.SaveAll(); },
			"Close", [](SaveGameManager& self, std::string_view name) { self.Close(name); },
			"CloseAll", [](SaveGameManager& self) { self.CloseAll(); },
			"IsOpen", [](SaveGameManager& self, std::string_view name) { return self.IsOpen(name); },
			"ExistsOnDisk", [](SaveGameManager& self, std::string_view name) { return self.ExistsOnDisk(name); },
			"DeleteFromDisk", [](SaveGameManager& self, std::string_view name) { return self.DeleteFromDisk(name); }
		);

		state["GameData"] = &Application::Instance().GetSaveGameManager();
	}

}
