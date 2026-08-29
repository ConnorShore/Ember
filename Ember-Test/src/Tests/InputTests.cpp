// InputCodeNames: the serialized names that land in .ebproj files, and the trigger-string grammar
// that binding overrides are keyed on. Pure string logic, so none of this needs a running engine.
//
// The round-trip tests are what keep KeyCodes.inl honest: the enum, these name tables and the Lua
// KeyCode enum are all generated from that one list, so a broken macro would silently desync them.

#include <Ember.h>

#include "TestFramework.h"

#include "Ember/Input/InputCodeNames.h"

#include <span>
#include <string>
#include <string_view>

using namespace Ember;
using Ember::Test::Type::Integration;
using Ember::Test::Type::Unit;

namespace {

	// The same lists the enums are generated from, so the Lua checks below compare the bindings
	// against the source of truth rather than against a second hand-written list.
	constexpr std::pair<const char*, int> s_KeyList[] =
	{
#define EB_KEY(name, value) { #name, value },
#include "Ember/Input/KeyCodes.inl"
#undef EB_KEY
	};

	constexpr std::pair<const char*, int> s_MouseControlList[] =
	{
#define EB_MOUSE_CONTROL(name, value) { #name, value },
#include "Ember/Input/MouseControls.inl"
#undef EB_MOUSE_CONTROL
	};

	// Parses a trigger string, asserting it succeeded. Returns a default trigger on failure so the
	// caller can keep going and report every broken case in one run rather than only the first.
	InputTrigger ParseTrigger(std::string_view text)
	{
		InputTrigger trigger{};
		EB_EXPECT_MSG(InputCodeNames::TriggerFromString(text, trigger),
			"failed to parse '" + std::string(text) + "'");

		return trigger;
	}

}

//////////////////////////////////////////////////////////////////////////
// Key names
//////////////////////////////////////////////////////////////////////////

// The guard on the X-macro. Every name written into a project file must map back to the same key,
// or a saved binding silently resolves to something else after a round trip.
EB_TEST_CASE(Input, KeyCodeStringRoundTripsForEveryCode, Unit)
{
	int checked = 0;

	for (int value = 0; value < static_cast<int>(KeyCode::Last); value++)
	{
		const KeyCode key = static_cast<KeyCode>(value);
		const std::string_view name = InputCodeNames::KeyCodeToString(key);

		// Gaps in the GLFW numbering fall through to "Unknown"; only real keys round-trip.
		if (name == "Unknown")
			continue;

		EB_EXPECT_MSG(InputCodeNames::KeyCodeFromString(name) == key,
			"'" + std::string(name) + "' did not round-trip");
		checked++;
	}

	EB_NOTE("round-tripped " + std::to_string(checked) + " key names");

	// A collapsed list would still pass the loop above, since it would simply iterate less.
	EB_EXPECT_GT(checked, 100);
}

EB_TEST_CASE(Input, KeyCodeFromStringRejectsNamesThatAreNotKeys, Unit)
{
	EB_EXPECT(InputCodeNames::KeyCodeFromString("Bogus") == KeyCode::Unknown);
	EB_EXPECT(InputCodeNames::KeyCodeFromString("") == KeyCode::Unknown);

	// Neither sentinel is a bindable key, so neither may resolve to itself.
	EB_EXPECT(InputCodeNames::KeyCodeFromString("Unknown") == KeyCode::Unknown);
	EB_EXPECT(InputCodeNames::KeyCodeFromString("Last") == KeyCode::Unknown);
}

// Display names are free to change; serialized names are not. This pins the ones a rebind menu
// would otherwise render as raw enumerators.
EB_TEST_CASE(Input, KeyCodeDisplayNameOverridesOnlyTheUnreadableOnes, Unit)
{
	EB_EXPECT(InputCodeNames::KeyCodeDisplayName(KeyCode::D1) == "1");
	EB_EXPECT(InputCodeNames::KeyCodeDisplayName(KeyCode::GraveAccent) == "`");
	EB_EXPECT(InputCodeNames::KeyCodeDisplayName(KeyCode::Backslash) == "\\");
	EB_EXPECT(InputCodeNames::KeyCodeDisplayName(KeyCode::LeftControl) == "Left Ctrl");
	EB_EXPECT(InputCodeNames::KeyCodeDisplayName(KeyCode::PageUp) == "Page Up");
	EB_EXPECT(InputCodeNames::KeyCodeDisplayName(KeyCode::NumPadDecimal) == "Numpad .");

	// Everything without an override falls through to the serialized name.
	EB_EXPECT(InputCodeNames::KeyCodeDisplayName(KeyCode::W) == "W");
	EB_EXPECT(InputCodeNames::KeyCodeDisplayName(KeyCode::Space) == "Space");
	EB_EXPECT(InputCodeNames::KeyCodeDisplayName(KeyCode::F11) == "F11");
}

//////////////////////////////////////////////////////////////////////////
// Mouse control names
//////////////////////////////////////////////////////////////////////////

// The same guard as the key list, for MouseControls.inl.
EB_TEST_CASE(Input, MouseControlStringRoundTripsForEveryControl, Unit)
{
	int checked = 0;

	for (int value = 0; value <= static_cast<int>(MouseControl::WheelDown); value++)
	{
		const MouseControl control = static_cast<MouseControl>(value);
		const std::string_view name = InputCodeNames::MouseControlToString(control);

		// The list leaves a gap between the button block and the wheel entries.
		if (name.empty())
			continue;

		MouseControl parsed{};
		EB_EXPECT_MSG(InputCodeNames::MouseControlFromString(name, parsed) && parsed == control,
			"'" + std::string(name) + "' did not round-trip");
		checked++;
	}

	EB_NOTE("round-tripped " + std::to_string(checked) + " mouse control names");
	EB_EXPECT_EQ(checked, 17);
}

EB_TEST_CASE(Input, MouseControlFromStringRejectsNamesThatAreNotControls, Unit)
{
	MouseControl parsed{};

	EB_EXPECT_FALSE(InputCodeNames::MouseControlFromString("Bogus", parsed));
	EB_EXPECT_FALSE(InputCodeNames::MouseControlFromString("", parsed));

	// MouseButton's sentinels are not MouseControl names.
	EB_EXPECT_FALSE(InputCodeNames::MouseControlFromString("Last", parsed));
	EB_EXPECT_FALSE(InputCodeNames::MouseControlFromString("Unknown", parsed));
}

EB_TEST_CASE(Input, MouseControlDisplayNameOverridesOnlyTheUnreadableOnes, Unit)
{
	EB_EXPECT(InputCodeNames::MouseControlDisplayName(MouseControl::Left) == "Left Mouse");
	EB_EXPECT(InputCodeNames::MouseControlDisplayName(MouseControl::Middle) == "Middle Mouse");
	EB_EXPECT(InputCodeNames::MouseControlDisplayName(MouseControl::WheelUp) == "Wheel Up");

	// The numbered buttons have no override and fall through to the serialized name.
	EB_EXPECT(InputCodeNames::MouseControlDisplayName(MouseControl::Button4) == "Button4");
	EB_EXPECT(InputCodeNames::MouseControlDisplayName(MouseControl::Button15) == "Button15");
}

// The first three controls have to keep lining up with MouseButton, because that is what lets a
// polled MouseButton be matched against a MouseControl-based trigger.
EB_TEST_CASE(Input, MouseControlAgreesWithMouseButtonForTheFirstThree, Unit)
{
	EB_EXPECT_EQ(static_cast<int>(MouseControl::Left), static_cast<int>(MouseButton::Left));
	EB_EXPECT_EQ(static_cast<int>(MouseControl::Right), static_cast<int>(MouseButton::Right));
	EB_EXPECT_EQ(static_cast<int>(MouseControl::Middle), static_cast<int>(MouseButton::Middle));
}

//////////////////////////////////////////////////////////////////////////
// Raw state: edges, latches, the wheel
//////////////////////////////////////////////////////////////////////////

namespace {

	// Input is global static, so a test that pokes it has to leave it clean for the next one.
	// ClearAllStates only zeroes the levels, so two frames are needed to drain latches and snapshots.
	struct InputStateGuard
	{
		InputStateGuard() { Settle(); }
		~InputStateGuard() { Settle(); }

		static void Settle()
		{
			Input::ClearAllStates();
			Input::BeginFrame();
			Input::BeginFrame();
		}
	};

}

EB_TEST_CASE(Input, MouseControlEdgeLatchesForExactlyOneFrame, Unit)
{
	InputStateGuard guard;

	// Button4 is one of the side buttons the old MouseButton-sized array could not represent.
	Input::SetMouseControlState(MouseControl::Button4, true);
	Input::BeginFrame();
	EB_EXPECT(Input::IsMouseControlDown(MouseControl::Button4));
	EB_EXPECT(Input::IsMouseControlPressed(MouseControl::Button4));
	EB_EXPECT_FALSE(Input::IsMouseControlReleased(MouseControl::Button4));

	Input::BeginFrame();
	EB_EXPECT(Input::IsMouseControlDown(MouseControl::Button4));
	EB_EXPECT_FALSE(Input::IsMouseControlPressed(MouseControl::Button4));

	Input::SetMouseControlState(MouseControl::Button4, false);
	Input::BeginFrame();
	EB_EXPECT_FALSE(Input::IsMouseControlDown(MouseControl::Button4));
	EB_EXPECT(Input::IsMouseControlReleased(MouseControl::Button4));
}

// A press and release delivered inside one glfwPollEvents batch leaves the level state back at
// zero, so without the latches the press would never be observed at all.
EB_TEST_CASE(Input, TapInsideOnePollIsNotLost, Unit)
{
	InputStateGuard guard;

	Input::SetKeyState(KeyCode::Space, true);
	Input::SetKeyState(KeyCode::Space, false);
	Input::BeginFrame();

	EB_EXPECT(Input::IsKeyPressed(KeyCode::Space));
	EB_EXPECT(Input::IsKeyReleased(KeyCode::Space));
	EB_EXPECT_FALSE(Input::IsKeyDown(KeyCode::Space));

	Input::BeginFrame();
	EB_EXPECT_FALSE(Input::IsKeyPressed(KeyCode::Space));
}

EB_TEST_CASE(Input, MouseTapInsideOnePollIsNotLost, Unit)
{
	InputStateGuard guard;

	Input::SetMouseControlState(MouseControl::Left, true);
	Input::SetMouseControlState(MouseControl::Left, false);
	Input::BeginFrame();

	EB_EXPECT(Input::IsMouseControlPressed(MouseControl::Left));
	EB_EXPECT(Input::IsMouseControlReleased(MouseControl::Left));
	EB_EXPECT_FALSE(Input::IsMouseControlDown(MouseControl::Left));
}

// The wheel has no held state and GLFW sends no release, so it is modelled as a one-frame pulse.
EB_TEST_CASE(Input, WheelIsAOneFramePulseAndIsNeverHeld, Unit)
{
	InputStateGuard guard;

	Input::SetMouseScrollOffset(Vector2f(0.0f, 1.0f));
	Input::BeginFrame();
	EB_EXPECT(Input::IsMouseControlPressed(MouseControl::WheelUp));
	EB_EXPECT(Input::IsMouseControlReleased(MouseControl::WheelUp));
	EB_EXPECT_FALSE(Input::IsMouseControlDown(MouseControl::WheelUp));
	EB_EXPECT_FALSE(Input::IsMouseControlPressed(MouseControl::WheelDown));

	// One notch is one pulse - it must not still be firing on the next frame.
	Input::BeginFrame();
	EB_EXPECT_FALSE(Input::IsMouseControlPressed(MouseControl::WheelUp));

	Input::SetMouseScrollOffset(Vector2f(0.0f, -1.0f));
	Input::BeginFrame();
	EB_EXPECT(Input::IsMouseControlPressed(MouseControl::WheelDown));
	EB_EXPECT_FALSE(Input::IsMouseControlPressed(MouseControl::WheelUp));
}

EB_TEST_CASE(Input, ZeroScrollDoesNotPulseTheWheel, Unit)
{
	InputStateGuard guard;

	Input::SetMouseScrollOffset(Vector2f(3.0f, 0.0f));   // horizontal scroll only
	Input::BeginFrame();

	EB_EXPECT_FALSE(Input::IsMouseControlPressed(MouseControl::WheelUp));
	EB_EXPECT_FALSE(Input::IsMouseControlPressed(MouseControl::WheelDown));
}

// The MouseButton queries are adapters over the MouseControl array, so they must agree.
EB_TEST_CASE(Input, MouseButtonQueriesMatchMouseControlQueries, Unit)
{
	InputStateGuard guard;

	Input::SetMouseControlState(MouseControl::Right, true);
	Input::BeginFrame();

	EB_EXPECT(Input::IsMouseButtonDown(MouseButton::Right));
	EB_EXPECT(Input::IsMouseButtonPressed(MouseButton::Right));
	EB_EXPECT_FALSE(Input::IsMouseButtonDown(MouseButton::Left));
}

// Modifiers are derived in BeginFrame rather than plumbed from GLFW. Alt and Super were missing,
// which would have made any Alt+ or Super+ trigger impossible to match.
EB_TEST_CASE(Input, ModifiersAreDerivedFromTheModifierKeys, Unit)
{
	InputStateGuard guard;

	Input::SetKeyState(KeyCode::LeftAlt, true);
	Input::SetKeyState(KeyCode::RightSuper, true);
	Input::BeginFrame();

	EB_EXPECT(Input::IsModifierActive(KeyModifier::Alt));
	EB_EXPECT(Input::IsModifierActive(KeyModifier::Super));
	EB_EXPECT_FALSE(Input::IsModifierActive(KeyModifier::Control));
	EB_EXPECT_FALSE(Input::IsModifierActive(KeyModifier::Shift));

	Input::SetKeyState(KeyCode::LeftControl, true);
	Input::BeginFrame();
	EB_EXPECT(Input::IsModifierActive(KeyModifier::Control));
}

//////////////////////////////////////////////////////////////////////////
// Trigger strings
//////////////////////////////////////////////////////////////////////////

// Binding overrides are keyed on these strings, so a trigger that does not serialize back to itself
// would make a player's rebind unresolvable after a save/load.
EB_TEST_CASE(Input, TriggerStringRoundTrips, Unit)
{
	const char* triggers[] =
	{
		"Key/Space",
		"Key/W",
		"Key/D1",
		"Key/GraveAccent",
		"Key/LeftControl",
		"Key/NumPadDecimal",
		"Key/Ctrl+S",
		"Key/Ctrl+Shift+S",
		"Key/Ctrl+Shift+Alt+Super+S",
		"Mouse/Left",
		"Mouse/Button4",
		"Mouse/WheelUp",
		"Mouse/WheelDown",
	};

	for (const char* text : triggers)
	{
		const InputTrigger trigger = ParseTrigger(text);
		const std::string serialized = InputCodeNames::TriggerToString(trigger);

		EB_EXPECT_MSG(serialized == text,
			std::string("'") + text + "' round-tripped to '" + serialized + "'");
	}
}

// Modifiers parse in any order but always serialize in one, so an override file stays stable no
// matter which order the capture UI happened to record them in.
EB_TEST_CASE(Input, TriggerModifierOrderIsNormalized, Unit)
{
	const InputTrigger reordered = ParseTrigger("Key/Shift+Ctrl+S");
	EB_EXPECT_EQ(InputCodeNames::TriggerToString(reordered), std::string("Key/Ctrl+Shift+S"));

	// Same trigger either way round.
	const InputTrigger canonical = ParseTrigger("Key/Ctrl+Shift+S");
	EB_EXPECT_EQ(InputCodeNames::TriggerToString(reordered), InputCodeNames::TriggerToString(canonical));
}

EB_TEST_CASE(Input, TriggerFromStringRejectsMalformedInput, Unit)
{
	const char* malformed[] =
	{
		"",                     // empty
		"Space",                // no device prefix
		"Key/",                 // device but no control
		"Key/Bogus",            // unknown key name
		"Key/Unknown",          // the sentinel is not bindable
		"Key/Ctrl+",            // modifier with nothing after it
		"Mouse/Bogus",          // unknown mouse control
		"Nonsense/W",           // unknown device
		"Gamepad/ButtonSouth",  // reserved: InputTrigger has no gamepad control type yet
	};

	for (const char* text : malformed)
	{
		InputTrigger trigger{};
		EB_EXPECT_MSG(!InputCodeNames::TriggerFromString(text, trigger),
			std::string("'") + text + "' should not have parsed");
	}
}

// The parser splits on '+' only when the token before it is a real modifier. Splitting naively would
// eat the trailing '+' of a half-axis name and mis-parse it once gamepad support lands.
EB_TEST_CASE(Input, TriggerParserDoesNotEatATrailingPlus, Unit)
{
	InputTrigger trigger{};

	EB_EXPECT_FALSE(InputCodeNames::TriggerFromString("Gamepad/LeftStickX+", trigger));

	// It must fail on the device, not by mistaking "LeftStickX" for a modifier.
	EB_EXPECT(trigger.Device == InputDevice::None);
}

EB_TEST_CASE(Input, UnboundTriggerHasNoSerializedForm, Unit)
{
	const InputTrigger unbound{};

	EB_EXPECT(InputCodeNames::TriggerToString(unbound).empty());
	EB_EXPECT_EQ(InputCodeNames::TriggerToDisplayName(unbound), std::string("Unbound"));
}

// A failed parse must leave the output in a defined state rather than a half-populated one, so a
// caller that ignores the return value cannot end up with a trigger bound to the wrong control.
EB_TEST_CASE(Input, FailedParseResetsTheOutputTrigger, Unit)
{
	InputTrigger trigger = ParseTrigger("Key/Ctrl+S");
	EB_CHECK(trigger.Device == InputDevice::Keyboard);

	EB_EXPECT_FALSE(InputCodeNames::TriggerFromString("Key/Bogus", trigger));
	EB_EXPECT(trigger.Device == InputDevice::None);
	EB_EXPECT_EQ(static_cast<int>(trigger.RequiredModifier), 0);
}

//////////////////////////////////////////////////////////////////////////
// Lua enum bindings
//////////////////////////////////////////////////////////////////////////

namespace {

	// Compares one Lua enum table against the C++ list it is generated from. Breakage here is
	// silent: a script asking for a missing enumerator just gets nil and compares false forever.
	void CheckLuaEnum(const char* tableName, std::span<const std::pair<const char*, int>> expected)
	{
		sol::state& lua = ScriptEngine::GetState();

		const sol::optional<sol::table> table = lua[tableName];
		EB_CHECK_MSG(table.has_value(), std::string(tableName) + " is not bound in Lua");

		int missing = 0;
		for (const auto& [name, value] : expected)
		{
			const sol::optional<int> bound = (*table)[name];
			if (!bound.has_value())
			{
				EB_EXPECT_MSG(false, std::string(tableName) + "." + name + " missing from Lua");
				missing++;
				continue;
			}

			EB_EXPECT_MSG(*bound == value,
				std::string(tableName) + "." + name + " is " + std::to_string(*bound)
					+ " in Lua but " + std::to_string(value) + " in C++");
		}

		EB_NOTE("checked " + std::to_string(expected.size()) + " " + tableName
			+ " enumerators, " + std::to_string(missing) + " missing");
	}

}

EB_TEST_CASE(Input, LuaKeyCodeEnumMatchesTheGeneratedList, Integration)
{
	CheckLuaEnum("KeyCode", s_KeyList);

	// Both sentinels are bound by hand either side of the include, so they need checking separately.
	sol::state& lua = ScriptEngine::GetState();
	EB_EXPECT(lua["KeyCode"]["Unknown"].get<int>() == static_cast<int>(KeyCode::Unknown));
	EB_EXPECT(lua["KeyCode"]["Last"].get<int>() == static_cast<int>(KeyCode::Last));
}

EB_TEST_CASE(Input, LuaMouseControlEnumMatchesTheGeneratedList, Integration)
{
	CheckLuaEnum("MouseControl", s_MouseControlList);
}

//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Input, TriggerDisplayNamesAreHumanReadable, Unit)
{
	EB_EXPECT_EQ(InputCodeNames::TriggerToDisplayName(ParseTrigger("Key/Space")), std::string("Space"));
	EB_EXPECT_EQ(InputCodeNames::TriggerToDisplayName(ParseTrigger("Key/D1")), std::string("1"));
	EB_EXPECT_EQ(InputCodeNames::TriggerToDisplayName(ParseTrigger("Key/LeftControl")), std::string("Left Ctrl"));
	EB_EXPECT_EQ(InputCodeNames::TriggerToDisplayName(ParseTrigger("Key/Ctrl+S")), std::string("Ctrl + S"));
	EB_EXPECT_EQ(InputCodeNames::TriggerToDisplayName(ParseTrigger("Key/Ctrl+Shift+S")), std::string("Ctrl + Shift + S"));
	EB_EXPECT_EQ(InputCodeNames::TriggerToDisplayName(ParseTrigger("Mouse/Left")), std::string("Left Mouse"));
	EB_EXPECT_EQ(InputCodeNames::TriggerToDisplayName(ParseTrigger("Mouse/WheelUp")), std::string("Wheel Up"));
}
