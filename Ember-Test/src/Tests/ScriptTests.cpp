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

EB_TEST_CASE(Script, ReferenceArrayPropertyIsParsedWithItsElementKind, Integration)
{
	// The parsed element kind picks which drag/drop payload the inspector accepts, so losing it
	// means the list silently rejects every drop.
	auto scriptAsset = LoadScriptAsset("ref_array_props.lua", R"(
		local Arrays = {}

		Arrays.PowerUpPrefabs = PrefabRefArray()
		Arrays.DeathSounds = AssetRefArray("AudioClip")
		Arrays.PatrolPoints = EntityRefArray()

		function Arrays:OnCreate(entity) end

		return Arrays
	)");
	EB_CHECK(scriptAsset != nullptr);

	const std::vector<ScriptProperty> properties = ScriptEngine::GetScriptProperties(scriptAsset);

	bool sawPrefabs = false, sawSounds = false, sawPoints = false;
	for (const ScriptProperty& property : properties)
	{
		if (property.Name == "PowerUpPrefabs")
		{
			sawPrefabs = true;
			EB_EXPECT_MSG(property.Type == ScriptPropertyType::ReferenceArray, "PowerUpPrefabs should be typed as ReferenceArray");
			EB_EXPECT_MSG(property.ReferenceKind == ScriptReferenceKind::Prefab, "PowerUpPrefabs lost its Prefab element kind");

			const std::vector<UUID>* values = std::get_if<std::vector<UUID>>(&property.Value);
			EB_EXPECT_MSG(values != nullptr && values->empty(), "a declared reference array should default to an empty UUID list");
		}
		else if (property.Name == "DeathSounds")
		{
			sawSounds = true;
			EB_EXPECT(property.Type == ScriptPropertyType::ReferenceArray);
			EB_EXPECT_MSG(property.ReferenceKind == ScriptReferenceKind::AudioClip, "AssetRefArray lost its declared element kind");
		}
		else if (property.Name == "PatrolPoints")
		{
			sawPoints = true;
			EB_EXPECT(property.Type == ScriptPropertyType::ReferenceArray);
			EB_EXPECT_MSG(property.ReferenceKind == ScriptReferenceKind::Entity, "EntityRefArray lost its Entity element kind");
		}
	}

	EB_EXPECT(sawPrefabs);
	EB_EXPECT(sawSounds);
	EB_EXPECT(sawPoints);
}

EB_TEST_CASE(Script, ReferenceArrayReachesLuaAsAnIndexableTable, Integration)
{
	// sol2 turns a std::vector<UUID> into opaque userdata, where # reports nothing and ipairs never
	// iterates - so every script reading a reference array would silently see an empty list.
	auto scriptAsset = LoadScriptAsset("ref_array_lua.lua", R"(
		local Refs = {}

		Refs.Prefabs = PrefabRefArray()

		function Refs:OnCreate(entity)
		    self.count = #self.Prefabs
		    self.iterated = 0
		    for _, prefab in ipairs(self.Prefabs) do
		        self.iterated = self.iterated + 1
		    end
		    self.firstIsValid = self.count > 0 and self.Prefabs[1]:IsValid() or false
		    self.lastIsValid = self.count > 0 and self.Prefabs[self.count]:IsValid() or false
		end

		function Refs:OnUpdate(entity, delta) end

		return Refs
	)");
	EB_CHECK(scriptAsset != nullptr);

	SceneFixture scene("ScriptReferenceArrayScene");
	Entity populated = MakeEntityAt(*scene, "Populated", Vector3f(0.0f));
	Entity untouched = MakeEntityAt(*scene, "Untouched", Vector3f(0.0f));

	populated.AttachComponent<ScriptComponent>(scriptAsset->GetUUID());
	untouched.AttachComponent<ScriptComponent>(scriptAsset->GetUUID());

	// A half-filled list is the realistic editor state.
	const std::vector<UUID> assigned = { UUID(0x1234ABCD), Constants::InvalidUUID };

	// Re-fetch after both attaches - the second one can reallocate the dense component array.
	auto& populatedScript = populated.GetComponent<ScriptComponent>();
	ScriptEngine::SetScriptReferenceArrayPropertyOverride(populatedScript, "Prefabs", assigned, ScriptReferenceKind::Prefab);

	ScriptEngine::BindAPI(scene.Ptr());
	Sys<ScriptSystem>()->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	{
		auto& refreshed = populated.GetComponent<ScriptComponent>();
		EB_CHECK(refreshed.Instance.valid());

		// If the array arrived as userdata, OnCreate died on `#` and left every field nil.
		EB_CHECK_MSG(refreshed.Instance["count"].valid(), "OnCreate did not complete - check the engine log for a Lua error");

		EB_EXPECT_MSG(refreshed.Instance["count"].get<int>() == 2, "# did not see both elements of the override list");
		EB_EXPECT_MSG(refreshed.Instance["iterated"].get<int>() == 2, "ipairs did not walk the override list");
		EB_EXPECT_MSG(refreshed.Instance["firstIsValid"].get<bool>(), "the assigned UUID did not survive the trip into Lua");
		EB_EXPECT_MSG(!refreshed.Instance["lastIsValid"].get<bool>(), "an unassigned slot should arrive as an invalid UUID");
	}

	{
		// No override: the declared empty array must still be a table, not nil.
		auto& refreshed = untouched.GetComponent<ScriptComponent>();
		EB_CHECK(refreshed.Instance.valid());
		EB_CHECK_MSG(refreshed.Instance["count"].valid(), "OnCreate did not complete on the unpopulated entity");
		EB_EXPECT_MSG(refreshed.Instance["count"].get<int>() == 0, "an unpopulated reference array should read as empty");
	}
}

EB_TEST_CASE(Script, ReferenceArrayOverrideSurvivesSerialization, Integration)
{
	// Reference arrays serialize as a sequence, not a scalar - a dropped round trip empties the pool.
	const std::string scenePath = Ember::Test::TempFile("ref_array_scene.ebs");
	const UUID scriptUUID(0x5150);
	const std::vector<UUID> assigned = { UUID(0xAAAA), UUID(0xBBBB), UUID(0xCCCC) };

	{
		SceneFixture source("RefArraySerializeScene");
		Entity entity = MakeEntityAt(*source, "Spawner", Vector3f(0.0f));
		auto& component = entity.AttachComponent<ScriptComponent>(scriptUUID);
		ScriptEngine::SetScriptReferenceArrayPropertyOverride(component, "Prefabs", assigned, ScriptReferenceKind::Prefab);

		SceneSerializer serializer(source.Shared());
		serializer.Serialize(scenePath);
	}

	SceneFixture target("RefArrayDeserializeScene");
	SceneSerializer serializer(target.Shared());
	serializer.Deserialize(scenePath);

	Entity restored = target->GetEntity("Spawner");
	EB_CHECK_MSG(restored.IsValid(), "the scripted entity did not survive the round trip");
	EB_CHECK_MSG(restored.ContainsComponent<ScriptComponent>(), "the restored entity lost its ScriptComponent");

	auto& restoredScript = restored.GetComponent<ScriptComponent>();
	auto overrideEntry = restoredScript.UserPropertyOverrides.find("Prefabs");
	EB_CHECK_MSG(overrideEntry != restoredScript.UserPropertyOverrides.end(), "the reference array override was not written or not read back");

	EB_EXPECT(overrideEntry->second.Type == ScriptPropertyType::ReferenceArray);
	EB_EXPECT_MSG(overrideEntry->second.ReferenceKind == ScriptReferenceKind::Prefab, "the element kind was lost in serialization");

	const std::vector<UUID>* values = std::get_if<std::vector<UUID>>(&overrideEntry->second.Value);
	EB_CHECK_MSG(values != nullptr, "the deserialized override does not hold a UUID list");
	EB_EXPECT_MSG(*values == assigned, "the restored list does not match what was saved, in contents or order");

	Ember::Test::RemoveTempFile(scenePath);
}

//////////////////////////////////////////////////////////////////////////
// Script inheritance (Base)
//////////////////////////////////////////////////////////////////////////

EB_TEST_CASE(Script, BaseFieldChainsMethodsFromParentScript, Integration)
{
	// A child script only has to declare Base; a method it doesn't define itself should still
	// resolve through the parent via the metatable chain ResolveScriptInheritance sets up on load.
	auto parentAsset = LoadScriptAsset("inherit_base_parent.lua", R"(
		local Parent = {}

		function Parent:GetParentValue()
		    return 42
		end

		return Parent
	)");
	EB_CHECK(parentAsset != nullptr);

	auto childAsset = LoadScriptAsset("inherit_base_child.lua", R"(
		local Child = {}
		Child.Base = "inherit_base_parent"

		function Child:OnCreate(entity)
		    self.inheritedValue = self:GetParentValue()
		end

		return Child
	)");
	EB_CHECK(childAsset != nullptr);

	SceneFixture scene("ScriptBaseInheritanceScene");
	Entity entity = MakeEntityAt(*scene, "Scripted", Vector3f(0.0f));
	entity.AttachComponent<ScriptComponent>(childAsset->GetUUID());

	ScriptEngine::BindAPI(scene.Ptr());
	Sys<ScriptSystem>()->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	auto& component = entity.GetComponent<ScriptComponent>();
	EB_CHECK_MSG(component.Initialized, "the child script was never initialized");
	EB_EXPECT_MSG(component.Instance["inheritedValue"].get<int>() == 42,
		"a method defined only on the Base script did not resolve on the child instance");
}

EB_TEST_CASE(Script, ChildScriptMethodOverridesParentMethod, Integration)
{
	// The child's own method must win over a same-named parent method - normal OOP override
	// semantics, not the parent silently shadowing the child's intent.
	auto parentAsset = LoadScriptAsset("inherit_override_parent.lua", R"(
		local Parent = {}
		function Parent:GetValue() return 1 end
		return Parent
	)");
	EB_CHECK(parentAsset != nullptr);

	auto childAsset = LoadScriptAsset("inherit_override_child.lua", R"(
		local Child = {}
		Child.Base = "inherit_override_parent"

		function Child:GetValue() return 2 end

		function Child:OnCreate(entity)
		    self.observedValue = self:GetValue()
		end

		return Child
	)");
	EB_CHECK(childAsset != nullptr);

	SceneFixture scene("ScriptBaseOverrideScene");
	Entity entity = MakeEntityAt(*scene, "Scripted", Vector3f(0.0f));
	entity.AttachComponent<ScriptComponent>(childAsset->GetUUID());

	ScriptEngine::BindAPI(scene.Ptr());
	Sys<ScriptSystem>()->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	auto& component = entity.GetComponent<ScriptComponent>();
	EB_EXPECT_MSG(component.Instance["observedValue"].get<int>() == 2,
		"the child's own override was shadowed by the parent's method");
}

EB_TEST_CASE(Script, MultiLevelBaseChainResolvesGrandparentMethods, Integration)
{
	// Base chains must walk more than one level: Child -> Parent -> Grandparent.
	auto grandparentAsset = LoadScriptAsset("inherit_multilevel_grandparent.lua", R"(
		local Grandparent = {}
		function Grandparent:GetGrandparentValue() return 7 end
		return Grandparent
	)");
	EB_CHECK(grandparentAsset != nullptr);

	auto parentAsset = LoadScriptAsset("inherit_multilevel_parent.lua", R"(
		local Parent = {}
		Parent.Base = "inherit_multilevel_grandparent"
		return Parent
	)");
	EB_CHECK(parentAsset != nullptr);

	auto childAsset = LoadScriptAsset("inherit_multilevel_child.lua", R"(
		local Child = {}
		Child.Base = "inherit_multilevel_parent"

		function Child:OnCreate(entity)
		    self.inheritedValue = self:GetGrandparentValue()
		end

		return Child
	)");
	EB_CHECK(childAsset != nullptr);

	SceneFixture scene("ScriptMultiLevelInheritanceScene");
	Entity entity = MakeEntityAt(*scene, "Scripted", Vector3f(0.0f));
	entity.AttachComponent<ScriptComponent>(childAsset->GetUUID());

	ScriptEngine::BindAPI(scene.Ptr());
	Sys<ScriptSystem>()->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	auto& component = entity.GetComponent<ScriptComponent>();
	EB_EXPECT_MSG(component.Instance["inheritedValue"].get<int>() == 7,
		"a method two levels up the Base chain (grandparent) did not resolve on the child instance");
}

EB_TEST_CASE(Script, BaseCycleIsDetectedAndDoesNotHangOrCrash, Integration)
{
	// A.Base = B and B.Base = A must not deadlock ResolveScriptInheritance in an infinite loop. The
	// cycle should be logged and walking should simply stop, leaving each script usable on its own.
	auto scriptB = LoadScriptAsset("inherit_cycle_b.lua", R"(
		local B = {}
		B.Base = "inherit_cycle_a"
		function B:GetOwnValue() return 2 end
		return B
	)");
	EB_CHECK(scriptB != nullptr);

	auto scriptA = LoadScriptAsset("inherit_cycle_a.lua", R"(
		local A = {}
		A.Base = "inherit_cycle_b"

		function A:GetOwnValue() return 1 end

		function A:OnCreate(entity)
		    self.ownValue = self:GetOwnValue()
		end

		return A
	)");
	EB_CHECK(scriptA != nullptr);

	SceneFixture scene("ScriptBaseCycleScene");
	Entity entity = MakeEntityAt(*scene, "Scripted", Vector3f(0.0f));
	entity.AttachComponent<ScriptComponent>(scriptA->GetUUID());

	ScriptEngine::BindAPI(scene.Ptr());
	Sys<ScriptSystem>()->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	auto& component = entity.GetComponent<ScriptComponent>();
	EB_CHECK_MSG(component.Initialized, "a Base cycle prevented the script from initializing at all");
	EB_EXPECT_MSG(component.Instance["ownValue"].get<int>() == 1,
		"the script's own method stopped working once its Base chain turned out to be cyclic");
}

EB_TEST_CASE(Script, MissingBaseScriptStillInitializesTheChildScript, Integration)
{
	// A typo'd or since-deleted Base name must not prevent the script itself from loading - it
	// should behave like a script with no Base at all, just without the (missing) inherited members.
	auto childAsset = LoadScriptAsset("inherit_missing_base_child.lua", R"(
		local Child = {}
		Child.Base = "ThisScriptDoesNotExist"

		function Child:OnCreate(entity)
		    self.created = true
		end

		return Child
	)");
	EB_CHECK(childAsset != nullptr);

	SceneFixture scene("ScriptMissingBaseScene");
	Entity entity = MakeEntityAt(*scene, "Scripted", Vector3f(0.0f));
	entity.AttachComponent<ScriptComponent>(childAsset->GetUUID());

	ScriptEngine::BindAPI(scene.Ptr());
	Sys<ScriptSystem>()->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	auto& component = entity.GetComponent<ScriptComponent>();
	EB_CHECK_MSG(component.Initialized, "a missing Base script prevented the child from initializing");
	EB_EXPECT_MSG(component.Instance["created"].get<bool>(), "OnCreate did not run when Base could not be resolved");
}

EB_TEST_CASE(Script, ExposedPropertiesInheritFromBaseScriptAndChildOverridesWin, Integration)
{
	// GetScriptProperties merges the Base chain's fields into the child's exposed-property list so
	// an inherited field (e.g. ItemCost) shows up in the Inspector without every subclass having to
	// redeclare it - but the child's own value for a shared field name must still win, and Base
	// itself must not leak in as a property.
	auto parentAsset = LoadScriptAsset("inherit_props_parent.lua", R"(
		local Parent = {}
		Parent.ItemCost = 10
		Parent.SharedFlag = true
		return Parent
	)");
	EB_CHECK(parentAsset != nullptr);

	auto childAsset = LoadScriptAsset("inherit_props_child.lua", R"(
		local Child = {}
		Child.Base = "inherit_props_parent"
		Child.ChildOnly = 5
		Child.SharedFlag = false

		function Child:OnCreate(entity) end

		return Child
	)");
	EB_CHECK(childAsset != nullptr);

	const std::vector<ScriptProperty> properties = ScriptEngine::GetScriptProperties(childAsset);

	bool sawBase = false, sawChildOnly = false, sawItemCost = false, sawSharedFlag = false;
	for (const ScriptProperty& property : properties)
	{
		if (property.Name == "Base")
			sawBase = true;
		else if (property.Name == "ChildOnly")
		{
			sawChildOnly = true;
			EB_EXPECT_MSG(std::get<int>(property.Value) == 5, "ChildOnly should keep the child's own value");
		}
		else if (property.Name == "ItemCost")
		{
			sawItemCost = true;
			EB_EXPECT_MSG(std::get<int>(property.Value) == 10, "ItemCost should be inherited from the Base script");
		}
		else if (property.Name == "SharedFlag")
		{
			sawSharedFlag = true;
			EB_EXPECT_MSG(std::get<bool>(property.Value) == false,
				"the child's own SharedFlag value should shadow the Base script's default");
		}
	}

	EB_EXPECT_MSG(!sawBase, "the Base field itself leaked into the exposed property list");
	EB_EXPECT(sawChildOnly);
	EB_EXPECT(sawItemCost);
	EB_EXPECT(sawSharedFlag);
}

EB_TEST_CASE(Script, GetScriptInstanceMatchesAnAncestorBaseScriptName, Integration)
{
	// Entity:GetScriptInstance(name) must recognize a name from anywhere in the script's Base
	// ancestry, not just its own concrete name - otherwise a caller can't treat every PickupItem/
	// PickupWeapon-like entity generically as "a PurchasableItem" without knowing the concrete type.
	auto baseAsset = LoadScriptAsset("gsi_ancestor_base.lua", R"(
		local Base = {}
		return Base
	)");
	EB_CHECK(baseAsset != nullptr);

	// The entity lookups run in OnUpdate, not OnCreate: every stable test in this file touches
	// `entity` from OnUpdate, and doing it from OnCreate turns out to be order-dependent here.
	auto childAsset = LoadScriptAsset("gsi_ancestor_child.lua", R"(
		local Child = {}
		Child.Base = "gsi_ancestor_base"

		function Child:OnUpdate(entity, delta)
		    self.matchedOwnName = entity:GetScriptInstance("gsi_ancestor_child") ~= nil
		    self.matchedBaseName = entity:GetScriptInstance("gsi_ancestor_base") ~= nil
		    self.matchedUnrelated = entity:GetScriptInstance("SomeUnrelatedScriptName") ~= nil
		end

		return Child
	)");
	EB_CHECK(childAsset != nullptr);

	SceneFixture scene("ScriptGetInstanceAncestorScene");
	Entity entity = MakeEntityAt(*scene, "Scripted", Vector3f(0.0f));
	entity.AttachComponent<ScriptComponent>(childAsset->GetUUID());

	ScriptEngine::BindAPI(scene.Ptr());
	Sys<ScriptSystem>()->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	auto& component = entity.GetComponent<ScriptComponent>();
	EB_CHECK(component.Instance.valid());
	EB_EXPECT_MSG(component.Instance["matchedOwnName"].get<bool>(), "GetScriptInstance did not match the script's own concrete name");
	EB_EXPECT_MSG(component.Instance["matchedBaseName"].get<bool>(), "GetScriptInstance did not match an ancestor Base name");
	EB_EXPECT_MSG(!component.Instance["matchedUnrelated"].get<bool>(), "GetScriptInstance matched a name unrelated to the script or its ancestry");
}

EB_TEST_CASE(Script, GetScriptInstanceMatchesEveryLevelOfAMultiLevelBaseChain, Integration)
{
	auto grandparentAsset = LoadScriptAsset("gsi_multilevel_grandparent.lua", R"(
		local Grandparent = {}
		function Grandparent:GrandparentMarker() return 1 end
		return Grandparent
	)");
	EB_CHECK(grandparentAsset != nullptr);

	auto parentAsset = LoadScriptAsset("gsi_multilevel_parent.lua", R"(
		local Parent = {}
		Parent.Base = "gsi_multilevel_grandparent"
		function Parent:ParentMarker() return 2 end
		return Parent
	)");
	EB_CHECK(parentAsset != nullptr);

	auto childAsset = LoadScriptAsset("gsi_multilevel_child.lua", R"(
		local Child = {}
		Child.Base = "gsi_multilevel_parent"

		function Child:OnUpdate(entity, delta)
		    self.matchedParent = entity:GetScriptInstance("gsi_multilevel_parent") ~= nil
		    self.matchedGrandparent = entity:GetScriptInstance("gsi_multilevel_grandparent") ~= nil
		end

		return Child
	)");
	EB_CHECK(childAsset != nullptr);

	SceneFixture scene("ScriptGetInstanceMultiLevelScene");
	Entity entity = MakeEntityAt(*scene, "Scripted", Vector3f(0.0f));
	entity.AttachComponent<ScriptComponent>(childAsset->GetUUID());

	ScriptEngine::BindAPI(scene.Ptr());
	Sys<ScriptSystem>()->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	auto& component = entity.GetComponent<ScriptComponent>();
	EB_CHECK(component.Instance.valid());
	EB_EXPECT_MSG(component.Instance["matchedParent"].get<bool>(), "GetScriptInstance did not match the immediate parent");
	EB_EXPECT_MSG(component.Instance["matchedGrandparent"].get<bool>(), "GetScriptInstance did not match the grandparent two levels up");
}

EB_TEST_CASE(Script, GetScriptInstanceStillWorksNormallyForAScriptWithNoBase, Integration)
{
	// A script with no Base at all must still only match its own name - confirms the always-stamped
	// __baseChain (now just [ownName]) doesn't change behavior for the common non-inheriting case.
	auto scriptAsset = LoadScriptAsset("gsi_no_base.lua", R"(
		local Standalone = {}

		function Standalone:OnUpdate(entity, delta)
		    self.matchedOwnName = entity:GetScriptInstance("gsi_no_base") ~= nil
		    self.matchedUnrelated = entity:GetScriptInstance("SomeUnrelatedScriptName") ~= nil
		end

		return Standalone
	)");
	EB_CHECK(scriptAsset != nullptr);

	SceneFixture scene("ScriptGetInstanceNoBaseScene");
	Entity entity = MakeEntityAt(*scene, "Scripted", Vector3f(0.0f));
	entity.AttachComponent<ScriptComponent>(scriptAsset->GetUUID());

	ScriptEngine::BindAPI(scene.Ptr());
	Sys<ScriptSystem>()->OnUpdate(Ember::Test::FixedStep(), scene.Ptr());

	auto& component = entity.GetComponent<ScriptComponent>();
	EB_EXPECT_MSG(component.Instance["matchedOwnName"].get<bool>(), "GetScriptInstance did not match its own name");
	EB_EXPECT_MSG(!component.Instance["matchedUnrelated"].get<bool>(), "GetScriptInstance matched an unrelated name");
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
