// InputCodeNames: the serialized names that land in .ebproj files, and the trigger-string grammar
// that binding overrides are keyed on. Pure string logic, so none of this needs a running engine.
//
// The round-trip tests are what keep KeyCodes.inl honest: the enum, these name tables and the Lua
// KeyCode enum are all generated from that one list, so a broken macro would silently desync them.

#include <Ember.h>

#include "TestFramework.h"
#include "TestHelpers.h"

#include "Ember/Input/InputActionManager.h"
#include "Ember/Input/InputCodeNames.h"

#include <span>
#include <string>
#include <string_view>
#include <vector>

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
// Gamepad control names
//////////////////////////////////////////////////////////////////////////

// The same guard as the key list, for GamepadButton.inl.
EB_TEST_CASE(Input, GamepadButtonStringRoundTripsForEveryButton, Unit)
{
	int checked = 0;

	for (int value = 0; value < static_cast<int>(GamepadButton::Last); value++)
	{
		const GamepadButton button = static_cast<GamepadButton>(value);
		const std::string_view name = InputCodeNames::GamepadButtonToString(button);

		if (name.empty())
		{
			EB_EXPECT_MSG(false, "button " + std::to_string(value) + " has no serialized name");
			continue;
		}

		GamepadButton parsed{};
		EB_EXPECT_MSG(InputCodeNames::GamepadButtonFromString(name, parsed) && parsed == button,
			"'" + std::string(name) + "' did not round-trip");
		checked++;
	}

	EB_NOTE("round-tripped " + std::to_string(checked) + " gamepad button names");
	EB_EXPECT_EQ(checked, 15);
}

EB_TEST_CASE(Input, GamepadAxisStringRoundTripsForEveryAxis, Unit)
{
	int checked = 0;

	for (int value = 0; value < static_cast<int>(GamepadAxis::Last); value++)
	{
		const GamepadAxis axis = static_cast<GamepadAxis>(value);
		const std::string_view name = InputCodeNames::GamepadAxisToString(axis);

		if (name.empty())
		{
			EB_EXPECT_MSG(false, "axis " + std::to_string(value) + " has no serialized name");
			continue;
		}

		GamepadAxis parsed{};
		EB_EXPECT_MSG(InputCodeNames::GamepadAxisFromString(name, parsed) && parsed == axis,
			"'" + std::string(name) + "' did not round-trip");
		checked++;
	}

	EB_NOTE("round-tripped " + std::to_string(checked) + " gamepad axis names");
	EB_EXPECT_EQ(checked, 6);
}

// The trigger parser picks button-or-axis from the control name alone, so the two name sets have to
// stay disjoint — an overlap would silently bind one control as the other.
EB_TEST_CASE(Input, GamepadButtonAndAxisNamesDoNotOverlap, Unit)
{
	for (int value = 0; value < static_cast<int>(GamepadButton::Last); value++)
	{
		const std::string_view name = InputCodeNames::GamepadButtonToString(static_cast<GamepadButton>(value));

		GamepadAxis axis{};
		EB_EXPECT_MSG(!InputCodeNames::GamepadAxisFromString(name, axis),
			"'" + std::string(name) + "' is both a button and an axis name");
	}

	for (int value = 0; value < static_cast<int>(GamepadAxis::Last); value++)
	{
		const std::string_view name = InputCodeNames::GamepadAxisToString(static_cast<GamepadAxis>(value));

		GamepadButton button{};
		EB_EXPECT_MSG(!InputCodeNames::GamepadButtonFromString(name, button),
			"'" + std::string(name) + "' is both an axis and a button name");
	}
}

EB_TEST_CASE(Input, GamepadNamesFromStringRejectNamesThatAreNotControls, Unit)
{
	GamepadButton button{};
	GamepadAxis axis{};

	EB_EXPECT_FALSE(InputCodeNames::GamepadButtonFromString("Bogus", button));
	EB_EXPECT_FALSE(InputCodeNames::GamepadButtonFromString("", button));
	EB_EXPECT_FALSE(InputCodeNames::GamepadAxisFromString("Bogus", axis));
	EB_EXPECT_FALSE(InputCodeNames::GamepadAxisFromString("", axis));

	// Neither sentinel is a bindable control.
	EB_EXPECT_FALSE(InputCodeNames::GamepadButtonFromString("Last", button));
	EB_EXPECT_FALSE(InputCodeNames::GamepadAxisFromString("Last", axis));
}

// Half-axis names are what a player actually picks in a rebind menu, and GLFW's Y axes are
// negative-up, which is the easy half to get backwards.
EB_TEST_CASE(Input, HalfAxisDisplayNamesFollowTheStickDirection, Unit)
{
	EB_EXPECT(InputCodeNames::GamepadAxisDisplayName(GamepadAxis::LeftX, AxisDirection::Negative) == "Left Stick Left");
	EB_EXPECT(InputCodeNames::GamepadAxisDisplayName(GamepadAxis::LeftX, AxisDirection::Positive) == "Left Stick Right");
	EB_EXPECT(InputCodeNames::GamepadAxisDisplayName(GamepadAxis::LeftY, AxisDirection::Negative) == "Left Stick Up");
	EB_EXPECT(InputCodeNames::GamepadAxisDisplayName(GamepadAxis::RightY, AxisDirection::Positive) == "Right Stick Down");

	// A whole axis, and a trigger that only moves one way, keep the plain name.
	EB_EXPECT(InputCodeNames::GamepadAxisDisplayName(GamepadAxis::LeftX, AxisDirection::Full) == "Left Stick X");
	EB_EXPECT(InputCodeNames::GamepadAxisDisplayName(GamepadAxis::LeftTrigger, AxisDirection::Positive) == "Left Trigger");
}

EB_TEST_CASE(Input, GamepadDisplayNamesAreControllerFacing, Unit)
{
	EB_EXPECT(InputCodeNames::GamepadButtonDisplayName(GamepadButton::A) == "A Button");
	EB_EXPECT(InputCodeNames::GamepadButtonDisplayName(GamepadButton::LeftBumper) == "Left Bumper");
	EB_EXPECT(InputCodeNames::GamepadButtonDisplayName(GamepadButton::LeftThumb) == "Left Stick Press");
	EB_EXPECT(InputCodeNames::GamepadButtonDisplayName(GamepadButton::DPadUp) == "D-Pad Up");

	// Back, Start and Guide read fine as enumerators and fall through to the serialized name.
	EB_EXPECT(InputCodeNames::GamepadButtonDisplayName(GamepadButton::Back) == "Back");
	EB_EXPECT(InputCodeNames::GamepadButtonDisplayName(GamepadButton::Start) == "Start");

	EB_EXPECT(InputCodeNames::GamepadAxisDisplayName(GamepadAxis::LeftX) == "Left Stick X");
	EB_EXPECT(InputCodeNames::GamepadAxisDisplayName(GamepadAxis::RightY) == "Right Stick Y");
	EB_EXPECT(InputCodeNames::GamepadAxisDisplayName(GamepadAxis::LeftTrigger) == "Left Trigger");
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
			// A zero offset pulses nothing, so this clears the wheel without leaving an edge behind.
			Input::SetMouseScrollOffset(Vector2f(0.0f, 0.0f));
			Input::SetGameplayInputSuppressed(false);
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
		"Gamepad/A",
		"Gamepad/LeftBumper",
		"Gamepad/DPadUp",
		"Gamepad/LeftX",
		"Gamepad/LeftX+",
		"Gamepad/LeftY-",
		"Gamepad/RightTrigger",
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
		"Gamepad/ButtonSouth",  // not one of Ember's gamepad control names
		"Gamepad/Last",         // the sentinel is not bindable
		"Gamepad/A+",           // only an axis has halves
		"Gamepad/+",            // a direction with no control
	};

	for (const char* text : malformed)
	{
		InputTrigger trigger{};
		EB_EXPECT_MSG(!InputCodeNames::TriggerFromString(text, trigger),
			std::string("'") + text + "' should not have parsed");
	}
}

// The suffix is what lets two actions split one stick axis, so it has to survive the parse rather
// than being swallowed as a modifier separator.
EB_TEST_CASE(Input, AxisTriggersCarryTheirDirection, Unit)
{
	EB_EXPECT(ParseTrigger("Gamepad/LeftX+").Direction == AxisDirection::Positive);
	EB_EXPECT(ParseTrigger("Gamepad/LeftY-").Direction == AxisDirection::Negative);

	// No suffix means the whole signed axis.
	EB_EXPECT(ParseTrigger("Gamepad/LeftX").Direction == AxisDirection::Full);

	// The control is the same either way; only the half differs.
	EB_EXPECT(std::get<GamepadAxis>(ParseTrigger("Gamepad/LeftX-").ControlId) == GamepadAxis::LeftX);

	// Non-axis triggers are left at Full.
	EB_EXPECT(ParseTrigger("Gamepad/A").Direction == AxisDirection::Full);
	EB_EXPECT(ParseTrigger("Key/Ctrl+S").Direction == AxisDirection::Full);
}

// The parser splits on '+' only when the token before it is a real modifier. Splitting naively would
// eat the trailing '+' of a half-axis name and mis-parse it once half-axis triggers land.
EB_TEST_CASE(Input, TriggerParserDoesNotEatATrailingPlus, Unit)
{
	InputTrigger trigger{};

	EB_EXPECT_FALSE(InputCodeNames::TriggerFromString("Gamepad/LeftStickX+", trigger));

	// It must fail on the control name, not by mistaking "LeftStickX" for a modifier.
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
	EB_EXPECT_EQ(static_cast<int>(trigger.RequiredModifiers), 0);
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
	EB_EXPECT_EQ(InputCodeNames::TriggerToDisplayName(ParseTrigger("Gamepad/A")), std::string("A Button"));
	EB_EXPECT_EQ(InputCodeNames::TriggerToDisplayName(ParseTrigger("Gamepad/DPadLeft")), std::string("D-Pad Left"));
	EB_EXPECT_EQ(InputCodeNames::TriggerToDisplayName(ParseTrigger("Gamepad/RightY")), std::string("Right Stick Y"));
	EB_EXPECT_EQ(InputCodeNames::TriggerToDisplayName(ParseTrigger("Gamepad/RightY-")), std::string("Right Stick Up"));
	EB_EXPECT_EQ(InputCodeNames::TriggerToDisplayName(ParseTrigger("Gamepad/LeftX+")), std::string("Left Stick Right"));
}

//////////////////////////////////////////////////////////////////////////
// Raw state: repeats, focus loss, mouse delta
//////////////////////////////////////////////////////////////////////////

// GLFW does not synthesize a PRESS for keys already held when the window regains focus, so a repeat
// arriving for a key we never saw pressed must not be able to fake a held key.
EB_TEST_CASE(Input, KeyRepeatOnlyCountsKeysWeSawPressed, Unit)
{
	InputStateGuard guard;

	Input::IncrementKeyRepeat(KeyCode::F);
	Input::BeginFrame();

	EB_EXPECT_FALSE(Input::IsKeyDown(KeyCode::F));
	EB_EXPECT_FALSE(Input::IsKeyRepeating(KeyCode::F));
	EB_EXPECT_EQ(Input::GetKeyRepeatCount(KeyCode::F), 0);
}

EB_TEST_CASE(Input, KeyRepeatsAccumulateOnlyWhileHeld, Unit)
{
	InputStateGuard guard;

	Input::SetKeyState(KeyCode::F, true);
	EB_EXPECT_FALSE(Input::IsKeyRepeating(KeyCode::F));   // down, but not yet repeating

	Input::IncrementKeyRepeat(KeyCode::F);
	EB_EXPECT(Input::IsKeyRepeating(KeyCode::F));
	EB_EXPECT_EQ(Input::GetKeyRepeatCount(KeyCode::F), 2);

	// Release drops the count outright rather than decaying it.
	Input::SetKeyState(KeyCode::F, false);
	EB_EXPECT_EQ(Input::GetKeyRepeatCount(KeyCode::F), 0);
	EB_EXPECT_FALSE(Input::IsKeyRepeating(KeyCode::F));
}

// A key released while the window is unfocused delivers no event, so without this the key stays
// stuck down for the rest of the session.
EB_TEST_CASE(Input, ClearAllStatesDropsHeldKeysMouseAndModifiers, Unit)
{
	InputStateGuard guard;

	Input::SetKeyState(KeyCode::W, true);
	Input::SetKeyState(KeyCode::LeftShift, true);
	Input::SetMouseControlState(MouseControl::Left, true);
	Input::BeginFrame();
	EB_CHECK(Input::IsKeyDown(KeyCode::W));
	EB_CHECK(Input::IsModifierActive(KeyModifier::Shift));

	Input::ClearAllStates();

	EB_EXPECT_FALSE(Input::IsKeyDown(KeyCode::W));
	EB_EXPECT_FALSE(Input::IsKeyDown(KeyCode::LeftShift));
	EB_EXPECT_FALSE(Input::IsMouseControlDown(MouseControl::Left));
	EB_EXPECT_EQ(static_cast<int>(Input::GetActiveModifiers()), 0);
}

// Regression: UpdateMousePosition rebaselined on every call, so only the last of a frame's cursor
// callbacks survived - a 1000Hz mouse at 60fps threw away about fifteen sixteenths of every look.
EB_TEST_CASE(Input, MouseDeltaAccumulatesEveryCallbackInTheFrame, Unit)
{
	const Vector2f savedPosition = Input::GetMousePosition();

	Input::UpdateMousePosition(Vector2f(100.0f, 100.0f));
	Input::ResetMouseDelta();

	// One frame's worth of reports, delivered as separate callbacks the way GLFW sends them.
	Input::UpdateMousePosition(Vector2f(103.0f, 100.0f));
	Input::UpdateMousePosition(Vector2f(107.0f, 102.0f));
	Input::UpdateMousePosition(Vector2f(110.0f, 105.0f));

	EB_EXPECT_VEC2_NEAR(Input::GetMouseDelta(), Vector2f(10.0f, 5.0f), 1e-4f);

	// The next frame starts from where this one ended, not from the last hop.
	Input::ResetMouseDelta();
	Input::UpdateMousePosition(Vector2f(112.0f, 105.0f));
	EB_EXPECT_VEC2_NEAR(Input::GetMouseDelta(), Vector2f(2.0f, 0.0f), 1e-4f);

	Input::UpdateMousePosition(savedPosition);
	Input::ResetMouseDelta();
}

// Regression: a 1.5px per-axis floor used to zero small movements, which is exactly the slow travel
// fine aim is made of, and it bent a slow diagonal onto whichever axis cleared the floor first.
EB_TEST_CASE(Input, MouseDeltaKeepsSlowMovement, Unit)
{
	const Vector2f savedPosition = Input::GetMousePosition();

	Input::UpdateMousePosition(Vector2f(100.0f, 100.0f));
	Input::ResetMouseDelta();

	Input::UpdateMousePosition(Vector2f(101.0f, 100.5f));
	EB_EXPECT_VEC2_NEAR(Input::GetMouseDelta(), Vector2f(1.0f, 0.5f), 1e-4f);

	Input::UpdateMousePosition(savedPosition);
	Input::ResetMouseDelta();
}

// Inversion is a device setting so every reader of the conditioned delta agrees, but the raw read
// stays true pixels for UI dragging and editor tooling.
EB_TEST_CASE(Input, MouseInversionAppliesToTheConditionedDeltaOnly, Unit)
{
	const Vector2f savedPosition = Input::GetMousePosition();
	const MouseSettings savedSettings = Input::GetMouseSettings();

	Input::GetMouseSettings().InvertY = true;

	Input::UpdateMousePosition(Vector2f(100.0f, 100.0f));
	Input::ResetMouseDelta();
	Input::UpdateMousePosition(Vector2f(110.0f, 120.0f));

	EB_EXPECT_VEC2_NEAR(Input::GetMouseDelta(), Vector2f(10.0f, -20.0f), 1e-4f);
	EB_EXPECT_VEC2_NEAR(Input::GetRawMouseDelta(), Vector2f(10.0f, 20.0f), 1e-4f);

	Input::GetMouseSettings() = savedSettings;
	Input::UpdateMousePosition(savedPosition);
	Input::ResetMouseDelta();
}

//////////////////////////////////////////////////////////////////////////
// Input actions
//////////////////////////////////////////////////////////////////////////

namespace {

	// Builds a manager with one action, spelled in the same grammar the project file stores.
	InputActionManager MakeManager(std::string_view actionName, std::string_view triggerText)
	{
		InputActionManager manager;
		manager.AddAction({ std::string(actionName), { ParseTrigger(triggerText) } });

		return manager;
	}

	// Samples a frame the way Application does: snapshot the raw state, then re-evaluate actions.
	void StepActions(InputActionManager& manager)
	{
		Input::BeginFrame();
		manager.Evaluate();
	}

}

EB_TEST_CASE(Input, ActionFollowsItsKeyThroughDownPressedReleased, Unit)
{
	InputStateGuard guard;
	InputActionManager actions = MakeManager("Jump", "Key/Space");

	Input::SetKeyState(KeyCode::Space, true);
	StepActions(actions);
	EB_EXPECT(actions.IsActionDown("Jump"));
	EB_EXPECT(actions.IsActionPressed("Jump"));
	EB_EXPECT_FALSE(actions.IsActionReleased("Jump"));

	// Still held, but the press edge is spent.
	StepActions(actions);
	EB_EXPECT(actions.IsActionDown("Jump"));
	EB_EXPECT_FALSE(actions.IsActionPressed("Jump"));

	Input::SetKeyState(KeyCode::Space, false);
	StepActions(actions);
	EB_EXPECT_FALSE(actions.IsActionDown("Jump"));
	EB_EXPECT(actions.IsActionReleased("Jump"));

	// The release edge is one frame too.
	StepActions(actions);
	EB_EXPECT_FALSE(actions.IsActionReleased("Jump"));
}

// Nothing changes until Evaluate runs, which is what keeps an action's edges stable for the whole
// frame no matter where in the frame a script asks.
EB_TEST_CASE(Input, ActionStateOnlyChangesOnEvaluate, Unit)
{
	InputStateGuard guard;
	InputActionManager actions = MakeManager("Jump", "Key/Space");

	Input::SetKeyState(KeyCode::Space, true);
	Input::BeginFrame();
	EB_EXPECT_FALSE(actions.IsActionDown("Jump"));

	actions.Evaluate();
	EB_EXPECT(actions.IsActionDown("Jump"));
}

EB_TEST_CASE(Input, ActionFiresForAnyOfItsTriggers, Unit)
{
	InputStateGuard guard;
	InputActionManager actions = MakeManager("Fire", "Key/Space");
	actions.AddTrigger("Fire", ParseTrigger("Mouse/Left"));

	Input::SetMouseControlState(MouseControl::Left, true);
	StepActions(actions);
	EB_EXPECT(actions.IsActionDown("Fire"));
	EB_EXPECT(actions.GetActionStates()[0].LastDevice == InputDevice::Mouse);

	// The key trigger is listed first, so it wins the scan and re-stamps the device.
	Input::SetMouseControlState(MouseControl::Left, false);
	Input::SetKeyState(KeyCode::Space, true);
	StepActions(actions);
	EB_EXPECT(actions.IsActionDown("Fire"));
	EB_EXPECT(actions.GetActionStates()[0].LastDevice == InputDevice::Keyboard);
}

EB_TEST_CASE(Input, ActionWithNoTriggersIsNeverDown, Unit)
{
	InputStateGuard guard;
	InputActionManager actions;
	actions.AddAction({ "Unbound", {} });

	Input::SetKeyState(KeyCode::Space, true);
	StepActions(actions);

	EB_EXPECT_FALSE(actions.IsActionDown("Unbound"));
	EB_EXPECT_FALSE(actions.IsActionPressed("Unbound"));
}

// Regression: IsTriggerActive tested only the held state, and a tap delivered entirely inside one
// glfwPollEvents batch leaves the level back at zero - so the action never saw the input at all.
EB_TEST_CASE(Input, ActionCatchesATapInsideOnePoll, Unit)
{
	InputStateGuard guard;
	InputActionManager actions = MakeManager("Interact", "Key/E");

	Input::SetKeyState(KeyCode::E, true);
	Input::SetKeyState(KeyCode::E, false);
	StepActions(actions);
	EB_EXPECT(actions.IsActionPressed("Interact"));

	StepActions(actions);
	EB_EXPECT_FALSE(actions.IsActionDown("Interact"));
	EB_EXPECT(actions.IsActionReleased("Interact"));
}

// Regression: the wheel is a one-frame pulse with no held state, so testing only the held state made
// every wheel binding silently dead - and the editor's trigger picker offers WheelUp/WheelDown.
EB_TEST_CASE(Input, WheelBoundActionFiresForOneFrame, Unit)
{
	InputStateGuard guard;
	InputActionManager actions = MakeManager("ZoomIn", "Mouse/WheelUp");

	Input::SetMouseScrollOffset(Vector2f(0.0f, 1.0f));
	StepActions(actions);
	EB_EXPECT(actions.IsActionDown("ZoomIn"));
	EB_EXPECT(actions.IsActionPressed("ZoomIn"));

	// One notch is one pulse: the action has to fall again on its own.
	StepActions(actions);
	EB_EXPECT_FALSE(actions.IsActionDown("ZoomIn"));
	EB_EXPECT(actions.IsActionReleased("ZoomIn"));
}

// RequiredModifiers is a subset test, not an equality test: every required modifier has to be held,
// and anything else held alongside them is ignored.
EB_TEST_CASE(Input, ActionRequiresAllOfItsModifiersAndToleratesOthers, Unit)
{
	InputStateGuard guard;
	InputActionManager actions = MakeManager("QuickSave", "Key/Ctrl+Shift+S");

	// One of the two is not enough.
	Input::SetKeyState(KeyCode::S, true);
	Input::SetKeyState(KeyCode::LeftControl, true);
	StepActions(actions);
	EB_EXPECT_FALSE(actions.IsActionDown("QuickSave"));

	Input::SetKeyState(KeyCode::LeftShift, true);
	StepActions(actions);
	EB_EXPECT(actions.IsActionDown("QuickSave"));

	// A modifier the trigger never asked for does not take the chord back out of range.
	Input::SetKeyState(KeyCode::LeftAlt, true);
	StepActions(actions);
	EB_EXPECT(actions.IsActionDown("QuickSave"));
}

// The point of the subset test: a plain-key action keeps firing while an unrelated modifier is
// held, so holding Shift to sprint does not silence a MoveForward bound to Key/W.
EB_TEST_CASE(Input, PlainKeyActionKeepsFiringWhileAModifierIsHeld, Unit)
{
	InputStateGuard guard;
	InputActionManager actions = MakeManager("MoveForward", "Key/W");

	Input::SetKeyState(KeyCode::W, true);
	StepActions(actions);
	EB_CHECK(actions.IsActionDown("MoveForward"));

	Input::SetKeyState(KeyCode::LeftShift, true);
	StepActions(actions);
	EB_EXPECT(actions.IsActionDown("MoveForward"));

	// It also must not re-fire its press edge, since it never went down in between.
	EB_EXPECT_FALSE(actions.IsActionPressed("MoveForward"));
}

// The cost of the subset test, and worth knowing before binding an editor-style chord: a trigger
// with no required modifiers matches unconditionally, so Ctrl+S fires the plain-S action too.
// Suppressing that needs the chord to win explicitly - the manager has no notion of priority.
EB_TEST_CASE(Input, ChordDoesNotSuppressThePlainKeyActionBoundToTheSameKey, Unit)
{
	InputStateGuard guard;
	InputActionManager actions = MakeManager("QuickSave", "Key/Ctrl+S");
	actions.AddAction({ "TypeS", { ParseTrigger("Key/S") } });

	Input::SetKeyState(KeyCode::S, true);
	StepActions(actions);
	EB_EXPECT_FALSE(actions.IsActionDown("QuickSave"));
	EB_EXPECT(actions.IsActionDown("TypeS"));

	Input::SetKeyState(KeyCode::LeftControl, true);
	StepActions(actions);
	EB_EXPECT(actions.IsActionDown("QuickSave"));
	EB_EXPECT(actions.IsActionDown("TypeS"));
	EB_NOTE("a chord and the plain-key action on the same key both fire; there is no suppression");
}

// Regression: GetActionIndex returns -1 for an unknown name, and the queries used to feed that
// straight into m_ActionStates[] - an out-of-bounds read on every typo'd action name in a script.
EB_TEST_CASE(Input, MissingActionQueriesReturnFalseInsteadOfIndexingOutOfRange, Unit)
{
	InputActionManager actions = MakeManager("Jump", "Key/Space");

	EB_EXPECT_EQ(actions.GetActionIndex("Nope"), -1);
	EB_EXPECT_FALSE(actions.IsActionDown("Nope"));
	EB_EXPECT_FALSE(actions.IsActionPressed("Nope"));
	EB_EXPECT_FALSE(actions.IsActionReleased("Nope"));

	// An empty manager is the same path with nothing to run off the end of.
	InputActionManager empty;
	EB_EXPECT_FALSE(empty.IsActionDown("Jump"));
}

// The by-name mutators route through the same -1, where an unguarded index is a bad erase or a
// straight out-of-bounds write rather than a read.
EB_TEST_CASE(Input, MissingActionMutatorsAreNoOps, Unit)
{
	InputActionManager actions = MakeManager("Jump", "Key/Space");

	actions.RemoveAction("Nope");
	actions.AddTrigger("Nope", ParseTrigger("Key/E"));
	actions.RemoveTrigger("Nope", 0);

	EB_EXPECT_EQ(actions.GetActions().size(), size_t(1));
	EB_EXPECT_EQ(actions.GetActionStates().size(), size_t(1));
	EB_EXPECT_EQ(actions.GetActions()[0].Triggers.size(), size_t(1));
}

// m_Actions and m_ActionStates are parallel vectors indexed in lockstep, so a removal that shrank
// only one of them would leave every later action reading someone else's state.
EB_TEST_CASE(Input, RemovingAnActionKeepsNamesAndStatesAligned, Unit)
{
	InputStateGuard guard;
	InputActionManager actions;
	actions.AddAction({ "First",  { ParseTrigger("Key/D1") } });
	actions.AddAction({ "Second", { ParseTrigger("Key/D2") } });
	actions.AddAction({ "Third",  { ParseTrigger("Key/D3") } });

	Input::SetKeyState(KeyCode::D3, true);
	StepActions(actions);
	EB_CHECK(actions.IsActionDown("Third"));

	actions.RemoveAction("First");
	StepActions(actions);

	EB_EXPECT_EQ(actions.GetActions().size(), size_t(2));
	EB_EXPECT_EQ(actions.GetActionStates().size(), size_t(2));
	EB_EXPECT(actions.IsActionDown("Third"));
	EB_EXPECT_FALSE(actions.IsActionDown("Second"));
}

EB_TEST_CASE(Input, EditActionNameRebindsTheLookup, Unit)
{
	InputActionManager actions = MakeManager("Jump", "Key/Space");

	actions.EditActionName(0, "Leap");

	EB_EXPECT_EQ(actions.GetActionIndex("Leap"), 0);
	EB_EXPECT_EQ(actions.GetActionIndex("Jump"), -1);
	EB_EXPECT_EQ(actions.GetActions()[0].Triggers.size(), size_t(1));
}

// Rebinding mid-play must not leave the action latched down against a trigger it no longer has.
EB_TEST_CASE(Input, EditingTriggersResetsTheActionState, Unit)
{
	InputStateGuard guard;
	InputActionManager actions = MakeManager("Fire", "Key/Space");

	Input::SetKeyState(KeyCode::Space, true);
	StepActions(actions);
	EB_CHECK(actions.IsActionDown("Fire"));

	actions.AddTrigger("Fire", ParseTrigger("Mouse/Left"));
	EB_EXPECT_FALSE(actions.GetActionStates()[0].IsDown);

	StepActions(actions);
	EB_CHECK(actions.IsActionDown("Fire"));

	actions.RemoveTrigger("Fire", 0);
	EB_EXPECT_EQ(actions.GetActions()[0].Triggers.size(), size_t(1));
	EB_EXPECT_FALSE(actions.GetActionStates()[0].IsDown);
}

EB_TEST_CASE(Input, ClearActionsEmptiesBothVectors, Unit)
{
	InputActionManager actions = MakeManager("Jump", "Key/Space");
	actions.AddAction({ "Fire", { ParseTrigger("Mouse/Left") } });

	actions.ClearActions();

	EB_EXPECT(actions.GetActions().empty());
	EB_EXPECT(actions.GetActionStates().empty());
	EB_EXPECT_EQ(actions.GetActionIndex("Jump"), -1);
}

// Regression: the UI acts on a press mid-frame, but Evaluate runs once at the top of it, so gameplay
// polling later in the same frame read the very same edge and one button press fired twice.
EB_TEST_CASE(Input, ConsumedActionStaysSilentUntilTheControlIsReleased, Unit)
{
	InputStateGuard guard;
	InputActionManager actions = MakeManager("Jump", "Key/Space");

	Input::SetKeyState(KeyCode::Space, true);
	StepActions(actions);
	EB_CHECK(actions.IsActionPressed("Jump"));

	actions.ConsumeControl(InputDevice::Keyboard, KeyCode::Space);
	EB_EXPECT_FALSE(actions.IsActionPressed("Jump"));
	EB_EXPECT_FALSE(actions.IsActionDown("Jump"));

	// Still held. Suppressing only the one frame would let the next Evaluate see a down that was
	// last recorded as up and call it a brand new press.
	StepActions(actions);
	EB_EXPECT_FALSE(actions.IsActionPressed("Jump"));
	EB_EXPECT_FALSE(actions.IsActionDown("Jump"));

	// The release goes with it: gameplay never saw the press, so a release would be a phantom.
	Input::SetKeyState(KeyCode::Space, false);
	StepActions(actions);
	EB_EXPECT_FALSE(actions.IsActionReleased("Jump"));

	// Consumption ends there - the next press is an ordinary one.
	Input::SetKeyState(KeyCode::Space, true);
	StepActions(actions);
	EB_EXPECT(actions.IsActionPressed("Jump"));
	EB_EXPECT(actions.IsActionDown("Jump"));
}

EB_TEST_CASE(Input, ConsumeOnlyTouchesActionsBoundToThatControl, Unit)
{
	InputStateGuard guard;
	InputActionManager actions;
	actions.AddAction({ "Jump", { ParseTrigger("Key/Space") } });
	actions.AddAction({ "Fire", { ParseTrigger("Mouse/Left") } });

	Input::SetKeyState(KeyCode::Space, true);
	Input::SetMouseControlState(MouseControl::Left, true);
	StepActions(actions);

	actions.ConsumeControl(InputDevice::Keyboard, KeyCode::Space);

	EB_EXPECT_FALSE(actions.IsActionPressed("Jump"));
	EB_EXPECT_MSG(actions.IsActionPressed("Fire"), "consuming one control silenced an unrelated action");
}

// The point of consuming by action: two actions on one physical control, where acting on the first
// must not leave the second free to act on the very same press.
EB_TEST_CASE(Input, ConsumeActionSilencesEveryActionOnTheControlThatFired, Unit)
{
	InputStateGuard guard;
	InputActionManager actions;
	actions.AddAction({ "Interact", { ParseTrigger("Key/E"), ParseTrigger("Mouse/Left") } });
	actions.AddAction({ "NavBack",  { ParseTrigger("Mouse/Left") } });

	Input::SetMouseControlState(MouseControl::Left, true);
	StepActions(actions);
	EB_CHECK(actions.IsActionPressed("Interact"));
	EB_CHECK(actions.IsActionPressed("NavBack"));

	actions.ConsumeAction("Interact");
	EB_EXPECT_FALSE(actions.IsActionPressed("Interact"));
	EB_EXPECT_MSG(!actions.IsActionPressed("NavBack"), "the other action on that control kept the press");

	// Still held, so the latch has to outlive the frame rather than handing the press over late.
	StepActions(actions);
	EB_EXPECT_FALSE(actions.IsActionPressed("NavBack"));
}

// Only the control that actually fired is taken away, so the action's other bindings keep working.
EB_TEST_CASE(Input, ConsumeActionLeavesTheTriggersThatDidNotFireAlone, Unit)
{
	InputStateGuard guard;
	InputActionManager actions;
	actions.AddAction({ "Interact", { ParseTrigger("Key/E"), ParseTrigger("Mouse/Left") } });
	actions.AddAction({ "TypeE",    { ParseTrigger("Key/E") } });

	Input::SetMouseControlState(MouseControl::Left, true);
	StepActions(actions);
	actions.ConsumeAction("Interact");

	// E never fired, so consuming the click must not have silenced the key-bound action too.
	Input::SetKeyState(KeyCode::E, true);
	StepActions(actions);
	EB_EXPECT(actions.IsActionPressed("TypeE"));
}

// LastControl still names whatever fired last, so consuming an idle action would silence a control
// the player is not touching.
EB_TEST_CASE(Input, ConsumeActionDoesNothingWhileTheActionIsIdle, Unit)
{
	InputStateGuard guard;
	InputActionManager actions = MakeManager("Interact", "Key/E");

	Input::SetKeyState(KeyCode::E, true);
	StepActions(actions);
	Input::SetKeyState(KeyCode::E, false);
	StepActions(actions);
	EB_CHECK_FALSE(actions.IsActionDown("Interact"));

	actions.ConsumeAction("Interact");

	Input::SetKeyState(KeyCode::E, true);
	StepActions(actions);
	EB_EXPECT(actions.IsActionPressed("Interact"));
}

EB_TEST_CASE(Input, ConsumeActionIgnoresAnUnknownName, Unit)
{
	InputStateGuard guard;
	InputActionManager actions = MakeManager("Interact", "Key/E");

	Input::SetKeyState(KeyCode::E, true);
	StepActions(actions);

	actions.ConsumeAction("Nope");
	EB_EXPECT(actions.IsActionPressed("Interact"));
}

// Regression: releasing the cursor to the editor left the game reading mouse travel, so the camera
// kept turning as the pointer crossed the editor UI.
EB_TEST_CASE(Input, SuppressedGameplayInputZeroesTheMouseDelta, Unit)
{
	InputStateGuard guard;
	const Vector2f savedPosition = Input::GetMousePosition();

	Input::UpdateMousePosition(Vector2f(100.0f, 100.0f));
	Input::ResetMouseDelta();

	Input::SetGameplayInputSuppressed(true);
	Input::UpdateMousePosition(Vector2f(140.0f, 130.0f));
	EB_EXPECT_VEC2_NEAR(Input::GetMouseDelta(), Vector2f(0.0f, 0.0f), 1e-4f);
	EB_EXPECT_VEC2_NEAR(Input::GetRawMouseDelta(), Vector2f(0.0f, 0.0f), 1e-4f);

	// Travel that happened while the editor held the pointer must not arrive when it hands back.
	Input::ResetMouseDelta();
	Input::SetGameplayInputSuppressed(false);
	EB_EXPECT_VEC2_NEAR(Input::GetMouseDelta(), Vector2f(0.0f, 0.0f), 1e-4f);

	Input::UpdateMousePosition(Vector2f(145.0f, 130.0f));
	EB_EXPECT_VEC2_NEAR(Input::GetMouseDelta(), Vector2f(5.0f, 0.0f), 1e-4f);

	Input::UpdateMousePosition(savedPosition);
	Input::ResetMouseDelta();
}

EB_TEST_CASE(Input, SuppressedGameplayInputLeavesActionsIdle, Unit)
{
	InputStateGuard guard;
	InputActionManager actions = MakeManager("MoveForward", "Key/W");

	Input::SetKeyState(KeyCode::W, true);
	StepActions(actions);
	EB_CHECK(actions.IsActionDown("MoveForward"));

	Input::SetGameplayInputSuppressed(true);
	StepActions(actions);
	EB_EXPECT_FALSE(actions.IsActionDown("MoveForward"));
	EB_EXPECT_FALSE(actions.IsActionPressed("MoveForward"));
	EB_EXPECT_NEAR(actions.GetActionStrength("MoveForward"), 0.0f, 0.0001f);
}

// A key held across the handover comes back held, not as a new press - otherwise clicking into the
// viewport with a key down would fire whatever that key triggers.
EB_TEST_CASE(Input, InputHandedBackIsHeldRatherThanAFreshPress, Unit)
{
	InputStateGuard guard;
	InputActionManager actions = MakeManager("Jump", "Key/Space");

	Input::SetKeyState(KeyCode::Space, true);
	StepActions(actions);
	EB_CHECK(actions.IsActionPressed("Jump"));

	Input::SetGameplayInputSuppressed(true);
	StepActions(actions);
	StepActions(actions);
	EB_CHECK_FALSE(actions.IsActionDown("Jump"));

	Input::SetGameplayInputSuppressed(false);
	StepActions(actions);
	EB_EXPECT(actions.IsActionDown("Jump"));
	EB_EXPECT_MSG(!actions.IsActionPressed("Jump"), "a key held across the handover fired a fresh press");
}

// The mirror image: a key let go while the editor held input must not deliver its release late.
EB_TEST_CASE(Input, ReleaseDuringSuppressionIsNotDeliveredLate, Unit)
{
	InputStateGuard guard;
	InputActionManager actions = MakeManager("Fire", "Key/Space");

	Input::SetKeyState(KeyCode::Space, true);
	StepActions(actions);
	EB_CHECK(actions.IsActionDown("Fire"));

	Input::SetGameplayInputSuppressed(true);
	StepActions(actions);
	Input::SetKeyState(KeyCode::Space, false);
	StepActions(actions);

	Input::SetGameplayInputSuppressed(false);
	StepActions(actions);
	EB_EXPECT_FALSE(actions.IsActionDown("Fire"));
	EB_EXPECT_MSG(!actions.IsActionReleased("Fire"), "a release from during the handover arrived late");
}

// Suppression is a separate gate from consumption, so it must not clear a latch that is still owed.
EB_TEST_CASE(Input, SuppressionDoesNotCancelAConsumedLatch, Unit)
{
	InputStateGuard guard;
	InputActionManager actions = MakeManager("Jump", "Key/Space");

	Input::SetKeyState(KeyCode::Space, true);
	StepActions(actions);
	actions.ConsumeAction("Jump");

	Input::SetGameplayInputSuppressed(true);
	StepActions(actions);
	Input::SetGameplayInputSuppressed(false);
	StepActions(actions);

	// Space never came up, so the consumed press is still owed and must stay swallowed.
	EB_EXPECT_FALSE(actions.IsActionDown("Jump"));
	EB_EXPECT_FALSE(actions.IsActionPressed("Jump"));
}

namespace {

	// Gamepad state is global and is only refreshed while a real pad is polled, so a test that pokes
	// it has to put it back for the next one.
	struct GamepadStateGuard
	{
		GamepadStateGuard() { Clear(); }
		~GamepadStateGuard() { Clear(); }

		static void Clear()
		{
			for (size_t i = 0; i < Input::MaxGamepads; i++)
			{
				GamepadState& state = Input::GetGamepadState(i);
				state.Down = 0;
				state.PreviousDown = 0;
				state.Axis.fill(0.0f);
				state.RawAxis.fill(0.0f);
				state.Connected = false;

				// A trigger rests at -1, not 0, so a zeroed array would read as half-pulled once a
				// test connects the pad and conditioning starts running.
				state.RawAxis[static_cast<size_t>(GamepadAxis::LeftTrigger)] = -1.0f;
				state.RawAxis[static_cast<size_t>(GamepadAxis::RightTrigger)] = -1.0f;
			}

			Input::GetSettings() = InputSettings{};
		}
	};

	// Conditioning only runs for a pad the platform layer says is plugged in, so a test that wants
	// the deadzone and curve applied has to claim one is.
	void ConnectPad(size_t index = 0)
	{
		Input::GetGamepadState(index).Connected = true;
	}

}

// Regression: the button mask was built from the button index instead of a shifted bit, so A (index
// 0) masked to nothing and could never fire, while B (1) tested A's bit.
EB_TEST_CASE(Input, GamepadButtonTriggerMatchesOnlyTheButtonItNames, Unit)
{
	InputStateGuard inputGuard;
	GamepadStateGuard gamepadGuard;

	InputActionManager actions = MakeManager("Fire", "Gamepad/A");

	Input::SetGamepadButtonPressed(0, GamepadButton::A);
	StepActions(actions);
	EB_EXPECT(actions.IsActionDown("Fire"));
	EB_EXPECT(actions.IsActionPressed("Fire"));

	Input::SetGamepadButtonReleased(0, GamepadButton::A);
	Input::SetGamepadButtonPressed(0, GamepadButton::B);
	StepActions(actions);
	EB_EXPECT_FALSE(actions.IsActionDown("Fire"));
	EB_EXPECT(actions.IsActionReleased("Fire"));
}

// Regression: Interact and NavBack shared Gamepad/B, so the press that opened a menu was still
// readable as a close by the menu's own check later in the same frame, and it never stayed open.
EB_TEST_CASE(Input, ConsumingASharedGamepadButtonStopsTheOpenAndCloseInOneFrame, Unit)
{
	InputStateGuard inputGuard;
	GamepadStateGuard gamepadGuard;

	InputActionManager actions;
	actions.AddAction({ "Interact", { ParseTrigger("Key/E"), ParseTrigger("Gamepad/B") } });
	actions.AddAction({ "NavBack",  { ParseTrigger("Gamepad/B") } });

	Input::SetGamepadButtonPressed(0, GamepadButton::B);
	StepActions(actions);
	EB_CHECK(actions.IsActionPressed("Interact"));

	// What the interaction script does the moment it dispatches the interact.
	actions.ConsumeAction("Interact");
	EB_EXPECT_MSG(!actions.IsActionPressed("NavBack"), "the interact press also read as a NavBack press");

	StepActions(actions);
	EB_EXPECT_FALSE(actions.IsActionPressed("NavBack"));

	// A fresh press is the player asking to close, and has to get through.
	Input::SetGamepadButtonReleased(0, GamepadButton::B);
	StepActions(actions);
	Input::SetGamepadButtonPressed(0, GamepadButton::B);
	StepActions(actions);
	EB_EXPECT(actions.IsActionPressed("NavBack"));
}

// The point of half-axis triggers: on one axis without a direction both actions would report the
// same value and GetAxis would cancel to zero no matter where the stick was.
EB_TEST_CASE(Input, HalfAxisTriggersSplitOneAxisIntoTwoActions, Unit)
{
	InputStateGuard inputGuard;
	GamepadStateGuard gamepadGuard;

	InputActionManager actions = MakeManager("MoveLeft", "Gamepad/LeftX-");
	actions.AddAction({ "MoveRight", { ParseTrigger("Gamepad/LeftX+") } });

	Input::SetGamepadAxis(0, GamepadAxis::LeftX, -0.5f);
	StepActions(actions);

	EB_EXPECT(actions.IsActionDown("MoveLeft"));
	EB_EXPECT_FALSE(actions.IsActionDown("MoveRight"));
	EB_EXPECT_NEAR(actions.GetActionStrength("MoveLeft"), 0.5f, 0.0001f);
	EB_EXPECT_NEAR(actions.GetAxis("MoveLeft", "MoveRight"), -0.5f, 0.0001f);

	Input::SetGamepadAxis(0, GamepadAxis::LeftX, 1.0f);
	StepActions(actions);

	EB_EXPECT(actions.IsActionDown("MoveRight"));
	EB_EXPECT_FALSE(actions.IsActionDown("MoveLeft"));
	EB_EXPECT_NEAR(actions.GetAxis("MoveLeft", "MoveRight"), 1.0f, 0.0001f);
}

// A whole-axis trigger keeps the sign, so an action bound to one reads as a signed axis on its own.
EB_TEST_CASE(Input, FullAxisTriggerReportsTheSignedValue, Unit)
{
	InputStateGuard inputGuard;
	GamepadStateGuard gamepadGuard;

	InputActionManager actions = MakeManager("Turn", "Gamepad/RightX");

	Input::SetGamepadAxis(0, GamepadAxis::RightX, -0.75f);
	StepActions(actions);

	EB_EXPECT(actions.IsActionDown("Turn"));
	EB_EXPECT_NEAR(actions.GetActionStrength("Turn"), -0.75f, 0.0001f);
}

EB_TEST_CASE(Input, StrongestTriggerSetsTheActionStrength, Unit)
{
	InputStateGuard inputGuard;
	GamepadStateGuard gamepadGuard;

	InputActionManager actions = MakeManager("MoveRight", "Key/D");
	actions.AddTrigger("MoveRight", ParseTrigger("Gamepad/LeftX+"));

	Input::SetGamepadAxis(0, GamepadAxis::LeftX, 0.4f);
	Input::SetKeyState(KeyCode::D, true);
	StepActions(actions);

	// The key is all the way down, so a half-pushed stick must not drag the strength back to 0.4.
	EB_EXPECT_NEAR(actions.GetActionStrength("MoveRight"), 1.0f, 0.0001f);
}

// Regression: a digital read of an axis fired on any non-zero value, so a stick pushed "straight"
// forward - which still leaves a few percent on the other axis - also held a strafe direction down,
// and a game reading IsActionDown could only ever move on 45 degree diagonals.
EB_TEST_CASE(Input, StickBleedIntoThePerpendicularAxisIsNotAPress, Unit)
{
	InputStateGuard inputGuard;
	GamepadStateGuard gamepadGuard;

	InputActionManager actions = MakeManager("MoveForward", "Gamepad/LeftY-");
	actions.AddAction({ "MoveRight", { ParseTrigger("Gamepad/LeftX+") } });

	Input::SetGamepadAxis(0, GamepadAxis::LeftY, -0.97f);
	Input::SetGamepadAxis(0, GamepadAxis::LeftX, 0.04f);
	StepActions(actions);

	EB_EXPECT(actions.IsActionDown("MoveForward"));
	EB_EXPECT_FALSE(actions.IsActionDown("MoveRight"));

	// The lean is still reported in full, so an analog reader keeps all 360 degrees of the stick.
	EB_EXPECT_NEAR(actions.GetActionStrength("MoveRight"), 0.04f, 0.0001f);

	// Past the actuation point it is a press like any other.
	Input::SetGamepadAxis(0, GamepadAxis::LeftX, 0.6f);
	StepActions(actions);
	EB_EXPECT(actions.IsActionDown("MoveRight"));
}

// A trigger has no cross-axis bleed to reject, so it must not inherit the stick's high bar - a light
// pull on the shoot trigger has to fire.
EB_TEST_CASE(Input, TriggerPressesWellBeforeTheStickActuationPoint, Unit)
{
	InputStateGuard inputGuard;
	GamepadStateGuard gamepadGuard;

	InputActionManager actions = MakeManager("Shoot", "Gamepad/RightTrigger");

	Input::SetGamepadAxis(0, GamepadAxis::RightTrigger, 0.3f);
	StepActions(actions);

	EB_EXPECT(actions.IsActionDown("Shoot"));
	EB_EXPECT_NEAR(actions.GetActionStrength("Shoot"), 0.3f, 0.0001f);
}

//////////////////////////////////////////////////////////////////////////
// Gamepad conditioning
//////////////////////////////////////////////////////////////////////////

// The defaults have to be the behaviour that existed before conditioning was configurable, or every
// project silently retunes itself on upgrade.
EB_TEST_CASE(Input, DefaultSettingsReproduceTheOriginalDeadzoneCurve, Unit)
{
	InputStateGuard inputGuard;
	GamepadStateGuard gamepadGuard;
	ConnectPad();

	const StickSettings& settings = Input::GetStickSettings(GamepadStick::Left);
	EB_EXPECT_NEAR(settings.Exponent, 1.0f, 1e-6f);
	EB_EXPECT_NEAR(settings.Saturation, 1.0f, 1e-6f);

	Input::GetGamepadState(0).RawAxis[static_cast<size_t>(GamepadAxis::LeftX)] = 0.5f;
	Input::BeginFrame();

	// The original rescale: (0.5 - 0.15) / (1 - 0.15).
	EB_EXPECT_NEAR(Input::GetGamepadAxis(0, GamepadAxis::LeftX), 0.35f / 0.85f, 1e-5f);

	// Inside the deadzone still reads as centred.
	Input::GetGamepadState(0).RawAxis[static_cast<size_t>(GamepadAxis::LeftX)] = 0.1f;
	Input::BeginFrame();
	EB_EXPECT_NEAR(Input::GetGamepadAxis(0, GamepadAxis::LeftX), 0.0f, 1e-6f);
}

// The whole point of shaping the magnitude instead of each axis: a per-axis Pow pulls a diagonal off
// its true angle, which is what makes a circular sweep of the stick feel lumpy.
EB_TEST_CASE(Input, StickCurveShapesMagnitudeWithoutBendingDirection, Unit)
{
	InputStateGuard inputGuard;
	GamepadStateGuard gamepadGuard;
	ConnectPad();

	Input::GetStickSettings(GamepadStick::Right).Exponent = 2.0f;
	Input::GetStickSettings(GamepadStick::Right).Deadzone = 0.0f;

	GamepadState& pad = Input::GetGamepadState(0);
	const float diagonal = 0.5f;
	pad.RawAxis[static_cast<size_t>(GamepadAxis::RightX)] = diagonal;
	pad.RawAxis[static_cast<size_t>(GamepadAxis::RightY)] = diagonal;
	Input::BeginFrame();

	const float x = Input::GetGamepadAxis(0, GamepadAxis::RightX);
	const float y = Input::GetGamepadAxis(0, GamepadAxis::RightY);

	EB_EXPECT_NEAR(x, y, 1e-5f);

	// Magnitude is squared; the 45 degree angle is untouched.
	const float rawMagnitude = Math::Length(Vector2f(diagonal, diagonal));
	EB_EXPECT_NEAR(Math::Length(Vector2f(x, y)), rawMagnitude * rawMagnitude, 1e-5f);
}

// BeginFrame legitimately runs more than once between polls, so conditioning reads RawAxis rather
// than shaping its own output a second time.
EB_TEST_CASE(Input, ConditioningIsIdempotentAcrossRepeatedFrames, Unit)
{
	InputStateGuard inputGuard;
	GamepadStateGuard gamepadGuard;
	ConnectPad();

	Input::GetStickSettings(GamepadStick::Right).Exponent = 2.0f;
	Input::GetGamepadState(0).RawAxis[static_cast<size_t>(GamepadAxis::RightX)] = 0.8f;

	Input::BeginFrame();
	const float once = Input::GetGamepadAxis(0, GamepadAxis::RightX);

	Input::BeginFrame();
	Input::BeginFrame();
	EB_EXPECT_NEAR(Input::GetGamepadAxis(0, GamepadAxis::RightX), once, 1e-6f);
}

// Inversion is per stick because UIInputSystem navigates menus with the left one - a global invert
// would flip menu navigation along with the camera.
EB_TEST_CASE(Input, StickInversionAppliesToOneAxisOfOneStick, Unit)
{
	InputStateGuard inputGuard;
	GamepadStateGuard gamepadGuard;
	ConnectPad();

	Input::GetStickSettings(GamepadStick::Right).InvertY = true;

	GamepadState& pad = Input::GetGamepadState(0);
	pad.RawAxis[static_cast<size_t>(GamepadAxis::RightX)] = 0.6f;
	pad.RawAxis[static_cast<size_t>(GamepadAxis::RightY)] = 0.6f;
	pad.RawAxis[static_cast<size_t>(GamepadAxis::LeftY)] = 0.6f;
	Input::BeginFrame();

	EB_EXPECT(Input::GetGamepadAxis(0, GamepadAxis::RightY) < 0.0f);
	EB_EXPECT(Input::GetGamepadAxis(0, GamepadAxis::RightX) > 0.0f);
	EB_EXPECT(Input::GetGamepadAxis(0, GamepadAxis::LeftY) > 0.0f);
}

// Actuation means physical travel, so it runs through the same curve the axis did. Without that, an
// exponent of 2 would quietly move every digital read on the stick from half throw to 0.71.
EB_TEST_CASE(Input, ActuationTracksPhysicalTravelThroughTheCurve, Unit)
{
	InputStateGuard inputGuard;
	GamepadStateGuard gamepadGuard;
	ConnectPad();

	StickSettings& settings = Input::GetStickSettings(GamepadStick::Left);
	settings.Deadzone = 0.0f;
	settings.Actuation = 0.5f;

	InputActionManager actions = MakeManager("MoveRight", "Gamepad/LeftX+");

	// Linear first: just short of half throw is not pressed, just past it is.
	Input::GetGamepadState(0).RawAxis[static_cast<size_t>(GamepadAxis::LeftX)] = 0.49f;
	StepActions(actions);
	EB_EXPECT_FALSE(actions.IsActionDown("MoveRight"));

	Input::GetGamepadState(0).RawAxis[static_cast<size_t>(GamepadAxis::LeftX)] = 0.51f;
	StepActions(actions);
	EB_EXPECT(actions.IsActionDown("MoveRight"));

	// Same physical travel still actuates once the stick is curved.
	settings.Exponent = 2.0f;
	Input::GetGamepadState(0).RawAxis[static_cast<size_t>(GamepadAxis::LeftX)] = 0.49f;
	StepActions(actions);
	EB_EXPECT_FALSE(actions.IsActionDown("MoveRight"));

	Input::GetGamepadState(0).RawAxis[static_cast<size_t>(GamepadAxis::LeftX)] = 0.51f;
	StepActions(actions);
	EB_EXPECT(actions.IsActionDown("MoveRight"));
}

// A trigger reports -1..1 and has to come out 0..1, so a released trigger must read as exactly zero
// rather than the half pull a naive remap of a zeroed axis would give.
EB_TEST_CASE(Input, TriggerRemapsRestingPositionToZero, Unit)
{
	InputStateGuard inputGuard;
	GamepadStateGuard gamepadGuard;
	ConnectPad();

	Input::BeginFrame();
	EB_EXPECT_NEAR(Input::GetGamepadAxis(0, GamepadAxis::RightTrigger), 0.0f, 1e-6f);

	// Fully pulled is +1 raw, and the deadzone rescale still has to land on 1.
	Input::GetGamepadState(0).RawAxis[static_cast<size_t>(GamepadAxis::RightTrigger)] = 1.0f;
	Input::BeginFrame();
	EB_EXPECT_NEAR(Input::GetGamepadAxis(0, GamepadAxis::RightTrigger), 1.0f, 1e-5f);
}

// A stick that cannot physically reach 1 would otherwise never drive anything at full rate.
EB_TEST_CASE(Input, SaturationReachesFullDeflectionEarly, Unit)
{
	InputStateGuard inputGuard;
	GamepadStateGuard gamepadGuard;
	ConnectPad();

	Input::GetStickSettings(GamepadStick::Right).Saturation = 0.9f;
	Input::GetGamepadState(0).RawAxis[static_cast<size_t>(GamepadAxis::RightX)] = 0.9f;
	Input::BeginFrame();

	EB_EXPECT_NEAR(Input::GetGamepadAxis(0, GamepadAxis::RightX), 1.0f, 1e-5f);
}

// ProjectSerializer stores each binding as its trigger string, so a saved action only survives if a
// trigger serializes and parses back to the same control - variant alternative included.
EB_TEST_CASE(Input, TriggerSurvivesTheSerializedRoundTrip, Unit)
{
	const char* triggers[] =
	{
		"Key/Space", "Key/Ctrl+Shift+S", "Mouse/Left", "Mouse/Button4", "Mouse/WheelDown",
		"Gamepad/A", "Gamepad/DPadLeft", "Gamepad/LeftY", "Gamepad/LeftY-", "Gamepad/LeftTrigger",
	};

	for (const char* text : triggers)
	{
		const InputTrigger original = ParseTrigger(text);

		InputTrigger restored{};
		EB_EXPECT_MSG(InputCodeNames::TriggerFromString(InputCodeNames::TriggerToString(original), restored),
			std::string("'") + text + "' did not parse back");

		EB_EXPECT_MSG(restored.Device == original.Device, std::string("'") + text + "' changed device");
		EB_EXPECT_MSG(restored.ControlId == original.ControlId, std::string("'") + text + "' changed control");
		EB_EXPECT_MSG(restored.RequiredModifiers == original.RequiredModifiers,
			std::string("'") + text + "' changed modifiers");
		EB_EXPECT_MSG(restored.Direction == original.Direction,
			std::string("'") + text + "' changed axis direction");
	}
}

//////////////////////////////////////////////////////////////////////////
// Lua input bindings
//////////////////////////////////////////////////////////////////////////

namespace {

	constexpr std::pair<const char*, int> s_KeyModifierList[] =
	{
		{ "None",    static_cast<int>(KeyModifier::None)    },
		{ "Shift",   static_cast<int>(KeyModifier::Shift)   },
		{ "Control", static_cast<int>(KeyModifier::Control) },
		{ "Alt",     static_cast<int>(KeyModifier::Alt)     },
		{ "Super",   static_cast<int>(KeyModifier::Super)   },
	};

	constexpr std::pair<const char*, int> s_MouseButtonList[] =
	{
		{ "Left",   static_cast<int>(MouseButton::Left)   },
		{ "Right",  static_cast<int>(MouseButton::Right)  },
		{ "Middle", static_cast<int>(MouseButton::Middle) },
	};

	// Runs a one-expression snippet against the shared editor state. script_pass_on_error keeps a
	// broken snippet a reported failure rather than a sol2 exception thrown through the test body.
	sol::protected_function_result RunLua(const char* source)
	{
		sol::protected_function_result result = ScriptEngine::GetState().script(source, sol::script_pass_on_error);
		EB_CHECK_MSG(result.valid(), std::string("lua failed: ") + source);

		return result;
	}

	bool LuaBool(const char* source) { return RunLua(source).get<bool>(0); }
	int LuaInt(const char* source) { return RunLua(source).get<int>(0); }

	// The Lua action queries reach through Application's manager, so a test driving them has to use
	// that one - and put back whatever the loaded project had in it.
	struct ScopedGlobalActions
	{
		ScopedGlobalActions() : Saved(Manager.GetActions()) { Manager.ClearActions(); }

		~ScopedGlobalActions()
		{
			Manager.ClearActions();
			for (const InputAction& action : Saved)
				Manager.AddAction(action);
		}

		InputActionManager& Manager = Application::Instance().GetInputActionManager();
		std::vector<InputAction> Saved;
	};

}

EB_TEST_CASE(Input, LuaKeyModifierEnumMatchesTheCppEnum, Integration)
{
	CheckLuaEnum("KeyModifier", s_KeyModifierList);
}

EB_TEST_CASE(Input, LuaMouseButtonAndCursorModeEnumsMatchTheCppEnums, Integration)
{
	CheckLuaEnum("MouseButton", s_MouseButtonList);

	sol::state& lua = ScriptEngine::GetState();
	EB_EXPECT_EQ(lua["CursorMode"]["Normal"].get<int>(), static_cast<int>(CursorMode::Normal));
	EB_EXPECT_EQ(lua["CursorMode"]["Hidden"].get<int>(), static_cast<int>(CursorMode::Hidden));
	EB_EXPECT_EQ(lua["CursorMode"]["Locked"].get<int>(), static_cast<int>(CursorMode::Locked));
}

// A misspelled binding name is silent from Lua: the call site indexes nil and the script dies at
// runtime, in whichever level happened to use it.
EB_TEST_CASE(Input, LuaInputTableExposesEveryQuery, Integration)
{
	const char* functions[] =
	{
		"IsKeyDown", "IsKeyPressed", "IsKeyReleased", "IsKeyRepeating",
		"IsMouseControlDown", "IsMouseControlPressed", "IsMouseControlReleased",
		"IsMouseButtonDown", "IsMouseButtonPressed", "IsMouseButtonReleased",
		"IsModifierDown", "IsModifierActive", "GetActiveModifiers",
		"IsActionDown", "IsActionPressed", "IsActionReleased", "ConsumeAction",
		"GetMousePosition", "GetMouseScrollOffset", "GetMouseDelta",
		"GetViewportMousePosition", "SetCursorMode", "GetCursorMode",
	};

	sol::state& lua = ScriptEngine::GetState();
	const sol::optional<sol::table> inputTable = lua["Input"];
	EB_CHECK_MSG(inputTable.has_value(), "Input is not bound in Lua");

	for (const char* name : functions)
	{
		const sol::object bound = (*inputTable)[name];
		EB_EXPECT_MSG(bound.valid() && bound.get_type() == sol::type::function,
			std::string("Input.") + name + " is not bound");
	}
}

EB_TEST_CASE(Input, LuaKeyAndMouseQueriesFollowTheRawInputState, Integration)
{
	InputStateGuard guard;

	Input::SetKeyState(KeyCode::W, true);
	Input::SetMouseControlState(MouseControl::Button4, true);
	Input::BeginFrame();

	EB_EXPECT(LuaBool("return Input.IsKeyDown(KeyCode.W)"));
	EB_EXPECT_FALSE(LuaBool("return Input.IsKeyDown(KeyCode.S)"));
	EB_EXPECT(LuaBool("return Input.IsMouseControlDown(MouseControl.Button4)"));
	EB_EXPECT(LuaBool("return Input.IsMouseControlPressed(MouseControl.Button4)"));
	EB_EXPECT_FALSE(LuaBool("return Input.IsMouseButtonDown(MouseButton.Left)"));

	Input::SetKeyState(KeyCode::W, false);
	Input::BeginFrame();
	EB_EXPECT(LuaBool("return Input.IsKeyReleased(KeyCode.W)"));
	EB_EXPECT_FALSE(LuaBool("return Input.IsKeyDown(KeyCode.W)"));
}

// Lua's IsKeyPressed is a legacy alias for the HELD state, not the one-frame edge the C++ function
// of that name returns. CharacterMovement.lua reads it as "is W down right now", so it stays.
EB_TEST_CASE(Input, LuaIsKeyPressedIsTheHeldStateNotTheEdge, Integration)
{
	InputStateGuard guard;

	Input::SetKeyState(KeyCode::W, true);
	Input::BeginFrame();
	EB_CHECK(LuaBool("return Input.IsKeyPressed(KeyCode.W)"));

	// Second frame: the C++ press edge is spent, but the Lua alias still reports true.
	Input::BeginFrame();
	EB_EXPECT_FALSE(Input::IsKeyPressed(KeyCode::W));
	EB_EXPECT(LuaBool("return Input.IsKeyPressed(KeyCode.W)"));
}

// Why IsMouseControlPressed has to be the edge query rather than another alias for the held state:
// the wheel has no held state, so aliasing it made scroll input invisible to scripts.
EB_TEST_CASE(Input, LuaSeesTheScrollWheelAsAPressedControl, Integration)
{
	InputStateGuard guard;

	Input::SetMouseScrollOffset(Vector2f(0.0f, 1.0f));
	Input::BeginFrame();

	EB_EXPECT(LuaBool("return Input.IsMouseControlPressed(MouseControl.WheelUp)"));
	EB_EXPECT_FALSE(LuaBool("return Input.IsMouseControlDown(MouseControl.WheelUp)"));
	EB_EXPECT_FALSE(LuaBool("return Input.IsMouseControlPressed(MouseControl.WheelDown)"));
}

EB_TEST_CASE(Input, LuaModifierQueriesFollowTheRawInputState, Integration)
{
	InputStateGuard guard;

	Input::SetKeyState(KeyCode::LeftShift, true);
	Input::SetKeyState(KeyCode::RightAlt, true);
	Input::BeginFrame();

	EB_EXPECT(LuaBool("return Input.IsModifierDown(KeyModifier.Shift)"));
	EB_EXPECT(LuaBool("return Input.IsModifierActive(KeyModifier.Alt)"));
	EB_EXPECT_FALSE(LuaBool("return Input.IsModifierDown(KeyModifier.Control)"));
	EB_EXPECT_FALSE(LuaBool("return Input.IsModifierDown(KeyModifier.Super)"));

	// None has no bits set, so it can never read as down.
	EB_EXPECT_FALSE(LuaBool("return Input.IsModifierDown(KeyModifier.None)"));

	EB_EXPECT_EQ(LuaInt("return Input.GetActiveModifiers()"), static_cast<int>(Input::GetActiveModifiers()));
}

// The mask is tested with AND, so a multi-bit argument asks "any of these". A chord that has to
// match exactly is compared against GetActiveModifiers() instead.
EB_TEST_CASE(Input, LuaMultiBitModifierArgumentMeansAnyOfThem, Integration)
{
	InputStateGuard guard;

	Input::SetKeyState(KeyCode::LeftControl, true);
	Input::BeginFrame();

	EB_EXPECT(LuaBool("return Input.IsModifierDown(KeyModifier.Shift | KeyModifier.Control)"));
	EB_EXPECT_FALSE(LuaBool("return Input.GetActiveModifiers() == (KeyModifier.Shift | KeyModifier.Control)"));
	EB_EXPECT(LuaBool("return Input.GetActiveModifiers() == KeyModifier.Control"));
}

EB_TEST_CASE(Input, LuaActionQueriesFollowTheActionManager, Integration)
{
	InputStateGuard guard;
	ScopedGlobalActions actions;

	actions.Manager.AddAction({ "Jump", { ParseTrigger("Key/Space") } });

	Input::SetKeyState(KeyCode::Space, true);
	Input::BeginFrame();
	actions.Manager.Evaluate();

	EB_EXPECT(LuaBool("return Input.IsActionDown('Jump')"));
	EB_EXPECT(LuaBool("return Input.IsActionPressed('Jump')"));
	EB_EXPECT_FALSE(LuaBool("return Input.IsActionReleased('Jump')"));

	Input::SetKeyState(KeyCode::Space, false);
	Input::BeginFrame();
	actions.Manager.Evaluate();

	EB_EXPECT_FALSE(LuaBool("return Input.IsActionDown('Jump')"));
	EB_EXPECT(LuaBool("return Input.IsActionReleased('Jump')"));
}

// The script-facing half of the shared-button fix: a script acts on the press, consumes it, and the
// action a different script polls on the same button stops seeing it.
EB_TEST_CASE(Input, LuaConsumeActionSilencesTheSharedControl, Integration)
{
	InputStateGuard guard;
	ScopedGlobalActions actions;

	actions.Manager.AddAction({ "Interact", { ParseTrigger("Key/E") } });
	actions.Manager.AddAction({ "NavBack",  { ParseTrigger("Key/E") } });

	Input::SetKeyState(KeyCode::E, true);
	Input::BeginFrame();
	actions.Manager.Evaluate();
	EB_CHECK(LuaBool("return Input.IsActionPressed('Interact')"));

	EB_EXPECT(LuaBool("Input.ConsumeAction('Interact') return not Input.IsActionPressed('NavBack')"));

	// A name no action has must not throw its way out of the binding.
	EB_EXPECT(LuaBool("Input.ConsumeAction('Nope') return true"));
}

// A typo in a script has to come back false rather than read past the end of the state array.
EB_TEST_CASE(Input, LuaQueriesForAnUnknownActionReturnFalse, Integration)
{
	ScopedGlobalActions actions;
	actions.Manager.AddAction({ "Jump", { ParseTrigger("Key/Space") } });

	EB_EXPECT_FALSE(LuaBool("return Input.IsActionDown('Jmup')"));
	EB_EXPECT_FALSE(LuaBool("return Input.IsActionPressed('Jmup')"));
	EB_EXPECT_FALSE(LuaBool("return Input.IsActionReleased('Jmup')"));
}
