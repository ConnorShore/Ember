// The C++/Lua boundary: bindings, script lifecycle, exposed properties and timers. Breakage here is
// silent - no compile error, the game just stops behaving.
// Test scripts are written to Ember-Test/tmp at run time so the Lua and its assertions live together.

#include <Ember.h>

#include "TestFramework.h"
#include "TestHelpers.h"

#include <fstream>
#include <stdexcept>
#include <string>

using namespace Ember;
using Ember::Test::Type::Integration;
using Ember::Test::Type::Unit;
using Ember::Test::Assets;
using Ember::Test::MakeEntityAt;
using Ember::Test::SceneFixture;
using Ember::Test::Sys;
using Ember::Test::TempFile;

namespace {

	// Writes a Lua source file into the scratch directory and returns its path.
	// Each test uses a UNIQUE filename: AssetManager::Load de-duplicates by absolute path, so
	// reusing a name would hand back the previously cached script with stale contents.
	std::string WriteScript(const std::string& fileName, const std::string& source)
	{
		const std::string path = TempFile(fileName);
		std::ofstream file(path, std::ios::trunc);
		if (file.is_open())
		{
			file << source;
			file.close();
		}
		return path;
	}

	SharedPtr<Script> LoadScriptAsset(const std::string& fileName, const std::string& source)
	{
		const std::string path = WriteScript(fileName, source);
		if (path.empty())
			return nullptr;
		return Assets().Load<Script>(path);
	}

} // namespace

//////////////////////////////////////////////////////////////////////////
// Math / core bindings (no scene required)
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Script, Vector3fBindingArithmetic, Unit)
{
	sol::state& lua = ScriptEngine::GetState();

	const sol::protected_function_result result = lua.script(R"(
		local a = Vector3f.new(1, 2, 3)
		local b = Vector3f.new(4, 5, 6)
		local sum = a + b
		local diff = b - a
		local scaled = a * 2.0
		return sum.x, sum.y, sum.z, diff.x, scaled.z
	)");

	EB_CHECK_MSG(result.valid(), "Vector3f binding script failed to run");

	EB_EXPECT_NEAR(result.get<float>(0), 5.0f, 1e-5); // sum.x
	EB_EXPECT_NEAR(result.get<float>(1), 7.0f, 1e-5); // sum.y
	EB_EXPECT_NEAR(result.get<float>(2), 9.0f, 1e-5); // sum.z
	EB_EXPECT_NEAR(result.get<float>(3), 3.0f, 1e-5); // diff.x
	EB_EXPECT_NEAR(result.get<float>(4), 6.0f, 1e-5); // scaled.z
}

EB_TEST_CASE(Script, MathTableMatchesTheCppFacade, Unit)
{
	// Lua sees the same Math implementation C++ does. If a binding is ever wired to the wrong
	// function, gameplay maths diverges from engine maths with no error anywhere.
	sol::state& lua = ScriptEngine::GetState();

	const sol::protected_function_result result = lua.script(R"(
		local v = Vector3f.new(3, 0, 4)
		return Math.Length(v),
		       Math.Dot(Vector3f.new(1,0,0), Vector3f.new(0,1,0)),
		       Math.Degrees(Math.Radians(90)),
		       Math.Lerp(0.0, 10.0, 0.25),
		       Math.Clamp(5.0, 0.0, 1.0),
		       Math.Distance(Vector3f.new(0,0,0), Vector3f.new(0,3,4))
	)");

	EB_CHECK_MSG(result.valid(), "Math table script failed to run");

	EB_EXPECT_NEAR(result.get<float>(0), 5.0f, 1e-4);   // Math.Length
	EB_EXPECT_NEAR(result.get<float>(1), 0.0f, 1e-5);   // Math.Dot of perpendicular axes
	EB_EXPECT_NEAR(result.get<float>(2), 90.0f, 1e-3);  // Degrees(Radians(90))
	EB_EXPECT_NEAR(result.get<float>(3), 2.5f, 1e-5);   // Math.Lerp
	EB_EXPECT_NEAR(result.get<float>(4), 1.0f, 1e-5);   // Math.Clamp
	EB_EXPECT_NEAR(result.get<float>(5), 5.0f, 1e-4);   // Math.Distance
}

EB_TEST_CASE(Script, NormalizeAndCrossMatchCpp, Unit)
{
	sol::state& lua = ScriptEngine::GetState();

	const sol::protected_function_result result = lua.script(R"(
		local n = Math.Normalize(Vector3f.new(0, 0, 5))
		local c = Math.Cross(Vector3f.new(1,0,0), Vector3f.new(0,1,0))
		return n.z, c.x, c.y, c.z
	)");

	EB_CHECK(result.valid());

	EB_EXPECT_NEAR(result.get<float>(0), 1.0f, 1e-5);
	EB_EXPECT_VEC3_NEAR(Vector3f(result.get<float>(1), result.get<float>(2), result.get<float>(3)),
		Math::Cross(Vector3f(1.0f, 0.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f)), 1e-5f);
}

EB_TEST_CASE(Script, LuaSyntaxErrorsAreReportedNotSwallowed, Unit)
{
	// sol2's protected_function_result must come back invalid on a bad script. If this ever
	// silently succeeded, broken gameplay scripts would fail with no diagnostic at all.
	sol::state& lua = ScriptEngine::GetState();
	const sol::protected_function_result result = lua.script("this is not valid lua ===", sol::script_pass_on_error);
	EB_EXPECT_FALSE(result.valid());
}

//////////////////////////////////////////////////////////////////////////
// Timers
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Script, SetTimeoutFiresOnceAfterItsDelay, Integration)
{
	sol::state& lua = ScriptEngine::GetState();
	ScriptEngine::ClearTimeouts();

	lua.script(R"(
		EmberTestTimeoutCount = 0
		function EmberTestTimeoutCallback() EmberTestTimeoutCount = EmberTestTimeoutCount + 1 end
	)");

	sol::protected_function callback = lua["EmberTestTimeoutCallback"];
	EB_CHECK(callback.valid());

	ScriptEngine::SetTimeout(callback, 0.5f);

	// Not yet due.
	ScriptEngine::UpdateTimeouts(TimeStep(0.2f));
	EB_EXPECT_EQ(lua["EmberTestTimeoutCount"].get<int>(), 0);

	// Crosses the threshold.
	ScriptEngine::UpdateTimeouts(TimeStep(0.4f));
	EB_EXPECT_EQ(lua["EmberTestTimeoutCount"].get<int>(), 1);

	// A fired timeout must be removed, not left to fire every frame afterwards.
	ScriptEngine::UpdateTimeouts(TimeStep(1.0f));
	EB_EXPECT_MSG(lua["EmberTestTimeoutCount"].get<int>() == 1, "the timeout fired more than once");

	ScriptEngine::ClearTimeouts();
}

EB_TEST_CASE(Script, ClearTimeoutsCancelsPendingCallbacks, Integration)
{
	sol::state& lua = ScriptEngine::GetState();
	ScriptEngine::ClearTimeouts();

	lua.script(R"(
		EmberTestCancelledFired = false
		function EmberTestCancelledCallback() EmberTestCancelledFired = true end
	)");

	sol::protected_function callback = lua["EmberTestCancelledCallback"];
	EB_CHECK(callback.valid());

	ScriptEngine::SetTimeout(callback, 0.1f);
	ScriptEngine::ClearTimeouts();
	ScriptEngine::UpdateTimeouts(TimeStep(1.0f));

	EB_EXPECT_FALSE(lua["EmberTestCancelledFired"].get<bool>());
}

//////////////////////////////////////////////////////////////////////////
// Script lifecycle on entities
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Script, OnCreateAndOnUpdateRunAndCanMutateTheEntity, Integration)
{
	// End-to-end: load a script asset, attach it, run one ScriptSystem tick, and confirm both hooks
	// fired AND that a write through the component binding landed on the real component.
	auto scriptAsset = LoadScriptAsset("lifecycle_test.lua", R"(
		local Lifecycle = {}

		function Lifecycle:OnCreate(entity)
		    self.createCount = (self.createCount or 0) + 1
		    self.updateCount = 0
		end

		function Lifecycle:OnUpdate(entity, delta)
		    self.updateCount = self.updateCount + 1
		    local t = entity:GetComponent("TransformComponent")
		    t.Position = Vector3f.new(self.updateCount, 0, 0)
		end

		return Lifecycle
	)");

	EB_CHECK_MSG(scriptAsset != nullptr, "failed to load the generated lifecycle test script");

	SceneFixture scene("ScriptLifecycleScene");
	Entity entity = MakeEntityAt(*scene, "Scripted", Vector3f(0.0f));
	entity.AttachComponent<ScriptComponent>(scriptAsset->GetUUID());

	// Bind the full gameplay API against this scene (Entity, components, Scene, Physics, ...).
	ScriptEngine::BindAPI(scene.Ptr());

	auto scriptSystem = Sys<ScriptSystem>();
	scriptSystem->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	{
		auto& component = entity.GetComponent<ScriptComponent>();
		EB_CHECK_MSG(component.Initialized, "the script was never initialized");
		EB_CHECK_MSG(component.Instance.valid(), "the per-entity script instance table is invalid");

		EB_EXPECT_MSG(component.Instance["createCount"].get<int>() == 1, "OnCreate did not run exactly once");
		EB_EXPECT_MSG(component.Instance["updateCount"].get<int>() == 1, "OnUpdate did not run on the first tick");
	}
	EB_EXPECT_VEC3_NEAR(entity.GetComponent<TransformComponent>().Position, Vector3f(1.0f, 0.0f, 0.0f), 1e-5f);

	// A second tick must run OnUpdate again WITHOUT re-running OnCreate. Component references are
	// re-fetched rather than held across the tick: a script is free to add components, and sparse-set
	// storage relocates when it does.
	scriptSystem->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	{
		auto& component = entity.GetComponent<ScriptComponent>();
		EB_EXPECT_MSG(component.Instance["createCount"].get<int>() == 1, "OnCreate ran a second time");
		EB_EXPECT_MSG(component.Instance["updateCount"].get<int>() == 2, "OnUpdate did not run on the second tick");
	}
	EB_EXPECT_VEC3_NEAR(entity.GetComponent<TransformComponent>().Position, Vector3f(2.0f, 0.0f, 0.0f), 1e-5f);
}

EB_TEST_CASE(Script, EachEntityGetsItsOwnInstanceTable, Integration)
{
	// Two entities sharing one script asset must NOT share `self`. If they did, per-entity state
	// (health, cooldowns, cached references) would bleed across every instance in the level.
	auto scriptAsset = LoadScriptAsset("instancing_test.lua", R"(
		local Counter = {}

		function Counter:OnCreate(entity)
		    self.ticks = 0
		end

		function Counter:OnUpdate(entity, delta)
		    self.ticks = self.ticks + 1
		end

		return Counter
	)");
	EB_CHECK(scriptAsset != nullptr);

	SceneFixture scene("ScriptInstancingScene");
	Entity first = MakeEntityAt(*scene, "First", Vector3f(0.0f));
	Entity second = MakeEntityAt(*scene, "Second", Vector3f(0.0f));
	first.AttachComponent<ScriptComponent>(scriptAsset->GetUUID());
	second.AttachComponent<ScriptComponent>(scriptAsset->GetUUID());

	ScriptEngine::BindAPI(scene.Ptr());
	auto scriptSystem = Sys<ScriptSystem>();

	scriptSystem->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());
	scriptSystem->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	auto& firstScript = first.GetComponent<ScriptComponent>();
	auto& secondScript = second.GetComponent<ScriptComponent>();
	EB_CHECK(firstScript.Instance.valid());
	EB_CHECK(secondScript.Instance.valid());

	EB_EXPECT_EQ(firstScript.Instance["ticks"].get<int>(), 2);
	EB_EXPECT_EQ(secondScript.Instance["ticks"].get<int>(), 2);

	// Mutating one instance must not be visible from the other.
	firstScript.Instance["ticks"] = 99;
	EB_EXPECT_MSG(secondScript.Instance["ticks"].get<int>() == 2, "the two entities share one instance table");
}

EB_TEST_CASE(Script, OnCreateThatSpawnsAScriptedEntityIsStillMarkedInitialized, Integration)
{
	// Regression: InitializeScriptForEntity held a ScriptComponent& across the OnCreate call, so an
	// OnCreate that spawns a scripted entity (Scene.InstantiatePrefab, DuplicateEntity) relocated the
	// packed storage and `Initialized = true` landed on the freed buffer. OnCreate then re-ran the
	// next tick with a fresh instance table, orphaning whatever the first one spawned.
	auto scriptAsset = LoadScriptAsset("oncreate_spawn_test.lua", R"(
		local Spawner = {}

		-- Initialized lazily inside OnCreate: InitializeScriptForEntity re-runs the file on every
		-- init, so a file-scope reset would wipe exactly the evidence this test is looking for.
		function Spawner:OnCreate(entity)
		    SpawnerOnCreateTotal = (SpawnerOnCreateTotal or 0) + 1
		    if not SpawnerHasSpawned then
		        SpawnerHasSpawned = true
		        Scene.DuplicateEntity("Spawner")
		    end
		end

		return Spawner
	)");
	EB_CHECK(scriptAsset != nullptr);

	SceneFixture scene("ScriptSpawnDuringOnCreateScene");
	Entity entity = MakeEntityAt(*scene, "Spawner", Vector3f(0.0f));
	entity.AttachComponent<ScriptComponent>(scriptAsset->GetUUID());

	// One scripted entity means the packed storage has capacity 1, so the duplicate's
	// ScriptComponent is guaranteed to reallocate it mid-OnCreate.
	ScriptEngine::BindAPI(scene.Ptr());
	auto scriptSystem = Sys<ScriptSystem>();
	scriptSystem->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	EB_CHECK_MSG(entity.GetComponent<ScriptComponent>().Initialized,
		"Initialized was written through a reference the OnCreate spawn had already invalidated");

	// Second tick initializes the duplicate. Two OnCreate calls total, one per entity - a third
	// means the original was re-initialized and has spawned an orphan nothing holds a handle to.
	scriptSystem->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	EB_EXPECT_MSG(ScriptEngine::GetState()["SpawnerOnCreateTotal"].get<int>() == 2,
		"OnCreate ran more than once per entity");
}

EB_TEST_CASE(Script, DisabledEntitiesDoNotTick, Integration)
{
	// ScriptSystem drives off ActiveQuery, so disabling an entity must stop its script. Otherwise
	// "disabled" enemies keep running their AI.
	auto scriptAsset = LoadScriptAsset("disabled_test.lua", R"(
		local Ticker = {}
		function Ticker:OnCreate(entity) self.ticks = 0 end
		function Ticker:OnUpdate(entity, delta) self.ticks = self.ticks + 1 end
		return Ticker
	)");
	EB_CHECK(scriptAsset != nullptr);

	SceneFixture scene("ScriptDisabledScene");
	Entity entity = MakeEntityAt(*scene, "Scripted", Vector3f(0.0f));
	entity.AttachComponent<ScriptComponent>(scriptAsset->GetUUID());

	ScriptEngine::BindAPI(scene.Ptr());
	auto scriptSystem = Sys<ScriptSystem>();

	scriptSystem->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());
	EB_CHECK_EQ(entity.GetComponent<ScriptComponent>().Instance["ticks"].get<int>(), 1);

	entity.SetActive(false);
	scriptSystem->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());
	scriptSystem->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());
	EB_EXPECT_MSG(entity.GetComponent<ScriptComponent>().Instance["ticks"].get<int>() == 1,
		"a disabled entity's script kept running");

	entity.SetActive(true);
	scriptSystem->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());
	EB_EXPECT_EQ(entity.GetComponent<ScriptComponent>().Instance["ticks"].get<int>(), 2);
}

EB_TEST_CASE(Script, ZeroDeltaSkipsTheTick, Integration)
{
	// ScriptSystem early-outs on a non-positive delta, which is how Pause is implemented. If that
	// check regressed, paused games would keep simulating.
	auto scriptAsset = LoadScriptAsset("paused_test.lua", R"(
		local Ticker = {}
		function Ticker:OnCreate(entity) self.ticks = 0 end
		function Ticker:OnUpdate(entity, delta) self.ticks = self.ticks + 1 end
		return Ticker
	)");
	EB_CHECK(scriptAsset != nullptr);

	SceneFixture scene("ScriptPausedScene");
	Entity entity = MakeEntityAt(*scene, "Scripted", Vector3f(0.0f));
	entity.AttachComponent<ScriptComponent>(scriptAsset->GetUUID());
	ScriptEngine::BindAPI(scene.Ptr());

	auto scriptSystem = Sys<ScriptSystem>();
	scriptSystem->OnUpdate(TimeStep(0.0f), scene.Ptr());

	// Nothing should even have been initialized.
	EB_EXPECT_FALSE(entity.GetComponent<ScriptComponent>().Initialized);

	scriptSystem->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());
	EB_EXPECT(entity.GetComponent<ScriptComponent>().Initialized);
}

//////////////////////////////////////////////////////////////////////////
// Editor-exposed properties
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Script, ExposedPropertiesAreParsedWithTheirTypes, Integration)
{
	// Every non-lifecycle field on the returned table becomes an inspector property. Type inference
	// (int vs float in particular, which needs a lua_isinteger check) drives which editor widget is
	// drawn and how the value is serialized.
	auto scriptAsset = LoadScriptAsset("properties_test.lua", R"(
		local Props = {}

		Props.Speed = 12.5
		Props.MaxHealth = 100
		Props.DisplayName = "Grunt"
		Props.IsHostile = true

		function Props:OnCreate(entity) end
		function Props:OnUpdate(entity, delta) end

		return Props
	)");
	EB_CHECK(scriptAsset != nullptr);

	const std::vector<ScriptProperty> properties = ScriptEngine::GetScriptProperties(scriptAsset);
	EB_CHECK_MSG(!properties.empty(), "no exposed properties were parsed from the script");

	bool sawSpeed = false, sawHealth = false, sawName = false, sawHostile = false, sawLifecycleHook = false;
	for (const ScriptProperty& property : properties)
	{
		if (property.Name == "Speed")
		{
			sawSpeed = true;
			EB_EXPECT_MSG(property.Type == ScriptPropertyType::Float, "Speed should be typed as Float");
		}
		else if (property.Name == "MaxHealth")
		{
			sawHealth = true;
			EB_EXPECT_MSG(property.Type == ScriptPropertyType::Int, "MaxHealth should be typed as Int, not Float");
		}
		else if (property.Name == "DisplayName")
		{
			sawName = true;
			EB_EXPECT_MSG(property.Type == ScriptPropertyType::String, "DisplayName should be typed as String");
		}
		else if (property.Name == "IsHostile")
		{
			sawHostile = true;
			EB_EXPECT_MSG(property.Type == ScriptPropertyType::Bool, "IsHostile should be typed as Bool");
		}
		else if (property.Name == "OnCreate" || property.Name == "OnUpdate")
		{
			sawLifecycleHook = true;
		}
	}

	EB_EXPECT(sawSpeed);
	EB_EXPECT(sawHealth);
	EB_EXPECT(sawName);
	EB_EXPECT(sawHostile);
	EB_EXPECT_MSG(!sawLifecycleHook, "a reserved lifecycle hook leaked into the exposed property list");
}

EB_TEST_CASE(Script, UserOverridesBeatScriptDefaults, Integration)
{
	// The inspector stores per-entity overrides on the component. They must be injected into the
	// instance table AFTER the script defaults, or every entity silently reverts to the default.
	auto scriptAsset = LoadScriptAsset("overrides_test.lua", R"(
		local Overridable = {}

		Overridable.Speed = 1.0
		Overridable.Label = "default"

		function Overridable:OnCreate(entity)
		    self.observedSpeed = self.Speed
		    self.observedLabel = self.Label
		end

		function Overridable:OnUpdate(entity, delta) end

		return Overridable
	)");
	EB_CHECK(scriptAsset != nullptr);

	SceneFixture scene("ScriptOverrideScene");
	Entity entity = MakeEntityAt(*scene, "Scripted", Vector3f(0.0f));
	auto& component = entity.AttachComponent<ScriptComponent>(scriptAsset->GetUUID());

	ScriptEngine::SetScriptPropertyOverride<float>(component, "Speed", 42.0f);
	ScriptEngine::SetScriptPropertyOverride<std::string>(component, "Label", std::string("overridden"));

	ScriptEngine::BindAPI(scene.Ptr());
	Sys<ScriptSystem>()->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	auto& refreshed = entity.GetComponent<ScriptComponent>();
	EB_CHECK(refreshed.Instance.valid());

	// OnCreate read the values, so the override must have been applied before it ran.
	EB_EXPECT_NEAR(refreshed.Instance["observedSpeed"].get<float>(), 42.0f, 1e-4);
	EB_EXPECT_EQ(refreshed.Instance["observedLabel"].get<std::string>(), std::string("overridden"));
}

//////////////////////////////////////////////////////////////////////////
// Error handling
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Script, RuntimeErrorInOnUpdateIsSurfaced, Integration)
{
	// ScriptSystem rethrows Lua errors as std::runtime_error so they cannot be silently ignored.
	// This test asserts that contract explicitly - and, just as importantly, that the failure is a
	// clean C++ exception rather than a crash or a corrupted Lua stack.
	auto scriptAsset = LoadScriptAsset("error_test.lua", R"(
		local Broken = {}
		function Broken:OnCreate(entity) end
		function Broken:OnUpdate(entity, delta)
		    error("deliberate test failure")
		end
		return Broken
	)");
	EB_CHECK(scriptAsset != nullptr);

	SceneFixture scene("ScriptErrorScene");
	Entity entity = MakeEntityAt(*scene, "Broken", Vector3f(0.0f));
	entity.AttachComponent<ScriptComponent>(scriptAsset->GetUUID());
	ScriptEngine::BindAPI(scene.Ptr());

	bool threw = false;
	try
	{
		Sys<ScriptSystem>()->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());
	}
	catch (const std::runtime_error&)
	{
		threw = true;
	}

	EB_EXPECT_MSG(threw, "a Lua error inside OnUpdate was swallowed instead of being raised");

	// The engine must still be usable afterwards - a poisoned Lua state would break every
	// subsequent test in the run.
	sol::state& lua = ScriptEngine::GetState();
	const sol::protected_function_result recovery = lua.script("return 1 + 1");
	EB_EXPECT_MSG(recovery.valid(), "the Lua state was left unusable after a script error");
}

EB_TEST_CASE(Script, MissingScriptHandleIsIgnored, Integration)
{
	// A ScriptComponent with no assigned script is a normal authoring state (the component was just
	// added in the inspector). It must be skipped quietly, not asserted on.
	SceneFixture scene("ScriptEmptyHandleScene");
	Entity entity = MakeEntityAt(*scene, "NoScript", Vector3f(0.0f));
	entity.AttachComponent<ScriptComponent>();

	ScriptEngine::BindAPI(scene.Ptr());
	Sys<ScriptSystem>()->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	EB_EXPECT_FALSE(entity.GetComponent<ScriptComponent>().Initialized);
}

//////////////////////////////////////////////////////////////////////////
// Save game binding
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Script, SaveFileGettersBindEveryArity, Integration)
{
	// Regression: sol2 does not honour C++ default arguments, so every arity has to be bound by
	// hand. When one is missed the shorter call dies with "expected string, received no value",
	// and a getter without a fallback is the form gameplay code reaches for most.
	SceneFixture scene("ScriptSaveGameArityScene");
	ScriptEngine::BindAPI(scene.Ptr());

	sol::state& lua = ScriptEngine::GetState();
	const sol::protected_function_result result = lua.script(R"(
		local file = GameData:Open("EmberTestArity")
		file:Clear()

		file:SetInt("Round", 4242)
		file:SetFloat("Ratio", 0.5)
		file:SetBool("Flag", true)
		file:SetString("Name", "Ember")

		-- Every getter called WITHOUT the trailing default argument.
		return file:GetInt("Round"), file:GetFloat("Ratio"), file:GetBool("Flag"), file:GetString("Name")
	)", sol::script_pass_on_error);

	EB_CHECK_MSG(result.valid(), "a save file getter rejected its shortest arity");

	EB_EXPECT_MSG(result.get<int>(0) == 4242, "GetInt(key) did not return the stored value");
	EB_EXPECT_NEAR(result.get<float>(1), 0.5f, 1e-6);
	EB_EXPECT_MSG(result.get<bool>(2), "GetBool(key) did not return the stored value");
	EB_EXPECT_EQ(result.get<std::string>(3), std::string("Ember"));
}

EB_TEST_CASE(Script, GameDataHandlesAddressSeparateFiles, Integration)
{
	// The point of the handle API: a script can keep a settings file and a score file open at once
	// and write the same key into both without either clobbering the other.
	SceneFixture scene("ScriptSaveGameSlotsScene");
	ScriptEngine::BindAPI(scene.Ptr());

	sol::state& lua = ScriptEngine::GetState();
	const sol::protected_function_result result = lua.script(R"(
		GameData:DeleteFromDisk("EmberTestScores")
		GameData:DeleteFromDisk("EmberTestSettings")
		GameData:CloseAll()

		local scores   = GameData:Open("EmberTestScores")
		local settings = GameData:Open("EmberTestSettings")

		scores:SetInt("Value", 10)
		settings:SetInt("Value", 20)
		settings:SetBool("Muted", true)

		local saved = GameData:SaveAll()
		GameData:CloseAll()

		-- Handles cached before CloseAll must report themselves dead rather than resolve.
		local staleIsValid = scores:IsValid()

		local reopenedScores   = GameData:Open("EmberTestScores")
		local reopenedSettings = GameData:Open("EmberTestSettings")

		return saved, staleIsValid,
			reopenedScores:GetInt("Value", -1),
			reopenedSettings:GetInt("Value", -1),
			reopenedSettings:GetBool("Muted", false)
	)", sol::script_pass_on_error);

	EB_CHECK_MSG(result.valid(), "the GameData handle API raised a Lua error");

	if (!result.get<bool>(0))
		EB_SKIP("SaveGameManager could not use the OS save directory");

	EB_EXPECT_MSG(result.get<bool>(1) == false, "a handle stayed valid after its file was closed");
	EB_EXPECT_MSG(result.get<int>(2) == 10, "the scores file did not round-trip its own value");
	EB_EXPECT_MSG(result.get<int>(3) == 20, "the settings file did not round-trip its own value");
	EB_EXPECT_MSG(result.get<bool>(4), "the settings file lost its bool value");
}
