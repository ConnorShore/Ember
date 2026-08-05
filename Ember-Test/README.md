# Ember-Test

A standalone test executable for the engine. It links `Ember`, boots a minimal `Application` (window
+ GL context + default assets), runs every registered test on the first frame, and exits with `0`
(all passed) or `1` (any failure) so CI can gate on it.

It exists both to test the engine and as **scaffolding**: copy the patterns here when adding tests.

## Running

Build `Ember-Test` (in Visual Studio, or with `scripts\Windows\Build\Build.bat Release Ember-Test`),
then:

```bat
scripts\Windows\Test\RunTests.bat                      REM Release, everything
scripts\Windows\Test\RunTests.bat Debug                REM Debug, everything
scripts\Windows\Test\RunTests.bat --filter=unit        REM only the fast unit tests
scripts\Windows\Test\RunTests.bat Debug --run=Physics  REM only Physics::* tests
scripts\Windows\Test\RunTests.bat --list               REM list tests without running them
```

Or run the executable directly — it must be launched **from the workspace root**, since asset paths
(`Ember/assets`, `Ember-Test/golden`, `Logs/`) are relative to it. Premake already sets `debugdir`
to the workspace root, so F5 from Visual Studio works.

> **Run it in Debug *and* Release.** Several engine bugs have only ever reproduced in optimized
> builds. `RunTests.bat` sets `EMBER_TEST_PERF_SCALE=8` automatically for Debug so the performance
> budgets measure regressions rather than the configuration difference.

> **After adding or removing a test file**, re-run `scripts\Windows\Build\GenerateProjects.bat`. Premake
> globs `src/**.cpp`, but the generated `.vcxproj` file list is not auto-synced with the filesystem.

### Options

Every option can be given as a command-line flag or an environment variable. Flags win.

| Flag | Environment variable | Effect |
|---|---|---|
| `--filter=<type>` | `EMBER_TEST_FILTER` | Run only one category: `unit`, `integration`, `visual`, `performance`, `stress`. |
| `--run=<substring>` | `EMBER_TEST_RUN` | Run only tests whose `Suite::Name` contains the substring (case-sensitive). |
| `--repeat=<n>` | `EMBER_TEST_REPEAT` | Run the whole selection N times. Good for hunting flaky tests and state leaking between runs. |
| `--list` | `EMBER_TEST_LIST` | Print every registered test and exit. |
| `--xml=<path>` | `EMBER_TEST_XML` | Write JUnit XML for CI. |
| `--perf-csv=<path>` | `EMBER_TEST_PERF_CSV` | Write every benchmark median to CSV so budgets can be tracked over time. |
| — | `EMBER_TEST_PERF_SCALE` | Multiplies every performance budget (default `1.0`). Use on slow machines or in Debug. |
| — | `EMBER_TEST_WRITE_GOLDEN` | Regenerate the visual reference images instead of comparing against them. |

### When a test crashes the process

There is no per-test process isolation, so a hard crash takes the whole run down — and in Debug an
engine `EB_CORE_ASSERT` calls `__debugbreak()`, which without a debugger attached *is* a crash.

`Logs/test-progress.log` is written and flushed **before** each test body runs. The last `RUNNING`
line in it names the test that died.

## Layout

| File | Covers |
|---|---|
| `src/TestFramework.h` | The framework: registration, assertions, benchmarking, reporting. No engine dependency. |
| `src/TestHelpers.h` | Engine-aware helpers: scene fixtures, `Vector3f`/`Matrix4f` comparisons, temp files. |
| `src/EmberTestApp.cpp` | The runner — boots the engine and executes the suite on frame one. |
| `src/Tests/UnitTests.cpp` | Math facade, UUID, `TimeStep`, filters, smart pointers, save games. |
| `src/Tests/EcsTests.cpp` | `Registry` / `ComponentManager` / `EntityManager` / `View`, lifecycle hooks, sparse-set integrity. |
| `src/Tests/SceneTests.cpp` | Entity identity, hierarchy, ordering, duplication, deferred removal, `CopyScene`. |
| `src/Tests/TransformTests.cpp` | `TransformSystem` propagation, dirty-flag behaviour, basis vectors. |
| `src/Tests/PhysicsTests.cpp` | Body/collider creation, simulation, raycasts, overlaps, runtime-pointer lifetime. |
| `src/Tests/AssetTests.cpp` | `AssetManager` lookup tables, asset round-trips, scene + prefab serialization. |
| `src/Tests/ScriptTests.cpp` | Lua bindings, script lifecycle, exposed properties, timers, error handling. |
| `src/Tests/AnimationTests.cpp` | Blackboard, condition evaluation, state machine, controller layers. |
| `src/Tests/AITests.cpp` | A* pathfinding, obstacle avoidance, corner-cutting rules, AI component defaults. |
| `src/Tests/RenderTests.cpp` | Camera projections, frustum culling, renderable bounds, visibility system, picking. |
| `src/Tests/AudioTests.cpp` | Move-only `AudioSourceComponent` through the sparse-set storage. |
| `src/Tests/PerfTests.cpp` | Performance budgets and relative-speed assertions. |
| `src/Tests/VisualTests.cpp` | Frame sanity, render determinism, golden images. |

## Test categories

The `TestType` on each test is orthogonal to which file it lives in — a file groups by *subsystem*,
the type controls *when CI runs it*.

| Type | Meaning | Needs a running engine? |
|---|---|---|
| `unit` | Pure logic. Fast, deterministic, runnable anywhere. Most numerous. | No |
| `integration` | Seams between subsystems: ECS ↔ systems, serialization round-trips. | Yes |
| `visual` | Renders a frame and asserts on pixels. | Yes (+ GL context) |
| `performance` | Asserts on a timing budget. Machine-dependent. | Yes |
| `stress` | Long or repeated runs hunting leaks and drift. | Yes |

## Writing a test

```cpp
#include <Ember.h>
#include "TestFramework.h"
#include "TestHelpers.h"

using namespace Ember;
using Ember::Test::Type::Integration;

EB_TEST_CASE(SuiteName, TestName, Integration)
{
    Ember::Test::SceneFixture scene("MyScene");
    Entity entity = Ember::Test::MakeEntityAt(*scene, "Thing", Vector3f(1.0f, 2.0f, 3.0f));
    scene.UpdateTransforms();

    EB_EXPECT_VEC3_NEAR(entity.GetComponent<TransformComponent>().GetWorldPosition(),
                        Vector3f(1.0f, 2.0f, 3.0f), 1e-4f);
}
```

Tests self-register through a static initializer — there is no list to maintain.

### Assertions

**`EB_CHECK*` is hard** — it throws, aborting the current test. Use it when continuing would be
meaningless (a null pointer you are about to dereference, a file that failed to write).

**`EB_EXPECT*` is soft** — it records the failure and keeps going, so one run reports *every* broken
field instead of only the first. Use it for independent value checks.

| Hard | Soft | |
|---|---|---|
| `EB_CHECK(cond)` | `EB_EXPECT(cond)` | |
| `EB_CHECK_FALSE(cond)` | `EB_EXPECT_FALSE(cond)` | |
| `EB_CHECK_EQ(a, b)` | `EB_EXPECT_EQ(a, b)` | |
| `EB_CHECK_NE(a, b)` | `EB_EXPECT_NE(a, b)` | |
| `EB_CHECK_NEAR(a, b, eps)` | `EB_EXPECT_NEAR(a, b, eps)` | |
| `EB_CHECK_MSG(cond, msg)` | `EB_EXPECT_MSG(cond, msg)` | message shown on failure |
| — | `EB_EXPECT_GT/GE/LT/LE(a, b)` | |
| `EB_CHECK_VEC3_NEAR` | `EB_EXPECT_VEC2/VEC3/VEC4_NEAR` | from `TestHelpers.h` |
| — | `EB_EXPECT_MAT4_NEAR(a, b, eps)` | |
| — | `EB_EXPECT_ROTATION_NEAR(qa, qb, eps)` | compares orientations: `q` and `-q` both pass |
| — | `EB_EXPECT_EULER_NEAR(ea, eb, eps)` | compares orientations, not Euler components |

Also:

- `EB_SKIP("reason")` — marks the test skipped rather than failed. For an absent optional fixture
  (no audio device, a golden image nobody has minted yet). Not for a test you expect to fail.
- `EB_NOTE("...")` — prints under the test line on pass *and* fail. Use it for measured numbers.

> **Compare rotations, never Euler components.** The same orientation has several valid Euler
> triples, so a component-wise compare is a classic source of flaky animation and transform tests.
> Use `EB_EXPECT_EULER_NEAR` / `EB_EXPECT_ROTATION_NEAR`.

### Benchmarks

```cpp
EB_BENCH_BUDGET("label", 5.0 /* ms */, 30 /* iterations */, {
    DoTheWork();
});

EB_BENCH_REPORT("label", 30, { DoTheWork(); });   // measure and print, do not assert
```

`Benchmark()` runs a warm-up pass and then reports the **median**, which shrugs off the occasional
OS scheduling hiccup that makes single-shot timings so flaky. Budgets are multiplied by
`EMBER_TEST_PERF_SCALE`.

Prefer **relative** comparisons where you can — they are machine-independent and are the only way to
prove an *optimisation* still works. `Perf::TransformDirtyFlagActuallySkipsWork` is the model: it
asserts a static transform pass is measurably cheaper than a fully dirty one, which is exactly what
would silently stop being true if the dirty-flag fast path regressed.

## Traps worth knowing about

These are engine behaviours, not framework quirks, and each has already caused a wrong test.

**Component references are invalidated by attaching components.** Storage is a sparse set backed by
`std::vector`, so attaching a component of type `T` can reallocate `T`'s dense array. Do not hold an
`auto&` to a component across any call that might create one — re-fetch instead.

**`Registry::AttachComponent<T>(entity, args...)` takes the entity first**; `Entity::AttachComponent<T>(args...)`
infers it. Mixing them up *compiles* for components whose first constructor parameter is numeric,
because a float silently converts to `EntityID`.

**Build the scene, then attach physics.** Rigid bodies are spawned from
`TransformComponent::WorldTransform`, which only `TransformSystem` populates. `PhysicsSceneFixture::Attach()`
encodes the right order (transform pass → `PhysicsSystem::OnSceneAttach`); attaching first spawns
every body at the origin.

**Attaching a scene destroys and rebuilds the whole rp3d world.** Any body pointer held across a
fixture boundary is dangling.

**`Scene::RemoveEntity` only queues the removal.** The queue is drained privately at the end of
`OnUpdateEdit`/`OnUpdateRuntime`, so a test asserting an entity is really gone must run a full frame
via `SceneFixture::TickEdit()`.

**Do not trip engine asserts in Debug.** `EB_CORE_ASSERT` is `__debugbreak()`. `AssetManager::GetAsset`
and `ComponentManager::GetComponent` both assert on a missing item, so guard with `ContainsAsset` /
`ContainsComponent` or use `Test::TryGetAsset`.

**`MaxEntities` is 1024 and `MaxComponents` is 64** (`Core/Constants.h`). Keep bulk tests under the
entity limit; `Ecs::ComponentTypeIdsAreDistinctAndInRange` guards the component limit.

**Never write `if (entity)` or `static_cast<bool>(entity)` — use `entity.IsValid()`.** `Entity` has a
non-explicit, non-const `operator EntityID()` next to its `explicit operator bool() const`. The
non-const conversion binds the implicit object argument better, so a bool cast silently calls
`operator EntityID()` and the result is **inverted in both directions**: a valid entity with handle
`0` reads as `false`, and an invalid entity (handle `1025`) reads as `true`. This cost 12 tests on
the first real run.

**The world origin is not empty in physics.** `PhysicsSystem::InitCameraSensor()` parks a persistent
kinematic 0.1-radius trigger sphere at the origin of *every* physics world, recreated on each
`RestartPhysicsWorld()`. It belongs to no entity and answers raycasts and overlap tests like anything
else — a downward ray through the origin hits it at y≈0.1 instead of your ground at y=0.
`PhysicsTests.cpp` runs every query in a `Lane()` offset away from the origin for this reason.
Ownerless objects now decode to `InvalidEntityID` (see `Physics/ColliderUserData.h`), so they are
distinguishable rather than masquerading as entity `0`.

**Never name a local `near` or `far`** — they are macros from the Windows headers.

**A `std::filesystem::path` read before its initialiser runs looks empty, not corrupt.** Static storage
is zero-filled before dynamic initialisation, and a zero-filled MSVC `path` is a valid *empty* path —
so the failure is a silently wrong path, never a crash. `Ember::Paths` originally had each accessor
own a function-local static whose initialiser called two other accessors' statics
(`EngineAssets()` → `IsInstalled()` → `ExecutableDir()`), and the chain was first entered from a
default argument in `WindowConfig.h`. The three accessors with that dependency chain returned empty
while the four without it were fine; every engine shader then failed to load from `shaders/...`
instead of `Ember/assets/shaders/...`, which surfaced as six *rendering* failures (flat frames,
garbage entity-ID picking, golden mismatches) and only four path failures. `Paths` now resolves
everything into one struct with no interdependencies, and logs the result via `Paths::LogResolved()` —
if assets ever fail to load, check those four lines at the top of `Logs/test.txt` first. Avoid
filesystem work in default arguments in widely-included headers for the same reason.

**The runner creates a throwaway `Project`, and some engine code requires one.** Ember-Forge and
Ember-Runtime always have an active project, and parts of the engine assume that without checking —
`ScriptEngine::BindAPI` → `BindPhysics` reaches through `ProjectManager::GetActive()` to the
collision filter table *at bind time*, so it null-derefs without one. `TestRunnerLayer::PrepareProject()`
stands one up under `Ember-Test/tmp/EmberTestProject/` before the suite runs. That is safe because
`AssetManager::ClearAssets()` (which `NewProject` calls) only drops **non-engine** assets, leaving
the default textures/shaders/meshes intact. If you add a test that exercises a new engine path,
consider whether it too assumes an active project.

**Read Lua values with `.get<T>()`, not a cast.** sol2's `table_proxy` exposes conversions through a
set of SFINAE-constrained `operator T()` / `operator T&()` overloads plus a non-template
`operator std::string()`. A C-style cast does not reliably pick the right one — MSVC rejects
`(std::string)table["key"]` outright (C2440). Write `table["key"].get<std::string>()`, which is the
documented idiom and is what those conversion operators call anyway.

```cpp
const int   ticks = instance["ticks"].get<int>();
const auto  label = instance["label"].get<std::string>();
```

**Indexing an `Entity` from Lua gets fragile as the suite goes on.** The runner keeps one long-lived
`sol::state`, and every script test binds the API onto it again — `ScriptEngine::BindAPI` → `BindEntity`
re-runs `state.new_usertype<Entity>` on each call. After enough of those, the `Entity` userdata handed
to a Lua hook stops being indexable and the hook dies with `attempt to index a sol.Ember::Entity value`.
The failure is *order-dependent*: the test passes alone and under a narrow `--run=` filter, then fails
in suite order, which makes it look like the engine change under test. Prefer asserting on the C++ side
or through Lua globals over calling `entity:GetName()` / `entity:GetComponent(...)` inside a test
script. The engine never hits this — `BindAPI` is only ever called from `ScriptEngine::OnRuntimeStart`,
immediately after a fresh `CreateConfiguredLuaState()`.

## Golden-image workflow

1. Mint the references once, from a **known-good** build:
   `set EMBER_TEST_WRITE_GOLDEN=1` then run.
2. Commit `Ember-Test/golden/*.png`.
3. Normal runs compare against them. A mismatch fails with the diff percentage. If the visual change
   was intended, regenerate and re-commit.

A golden that does not exist yet **skips** rather than fails, so a fresh clone is not drowned in
failures before anyone has minted them.

Goldens are GPU- and driver-dependent, so a reference minted on one machine may legitimately differ
on another. That is why `VisualTests.cpp` also carries two check styles that need no reference image
at all and can never fail spuriously:

- **Frame sanity** — the frame is not fully black, not blown out to white, and has real tonal
  variation. Catches the catastrophic failures from the very first run, on any machine.
- **Determinism** — the same scene rendered twice produces the same pixels. Fully
  machine-independent, and it catches uninitialised or leftover state directly.

Camera parameters in `RenderSceneToBackbuffer` (FOV, near/far clip) are part of the goldens'
identity — changing them invalidates every committed reference.

## Notes / next steps

- Loading `.ebs` scenes from disk (rather than code-built scenes) is a natural next set of tests;
  the serialization round-trips in `AssetTests.cpp` are the seed for that.
- The character controller, particle system, UI layout system and navigation-mesh baking have no
  direct coverage yet.
- Animation *sampling* and skinning are only covered indirectly through the visual tests, because
  they need real skeleton and clip assets. Committing a small rigged test asset would unlock them.
- For a richer framework (fixtures, parameterized tests), dropping in single-header **doctest** or
  **Catch2** would keep the existing test bodies almost unchanged.
- `RenderSystem::CaptureBackbufferToPNG` and `ComputeBackbufferStats` double as screenshot and
  frame-diagnostic utilities outside of testing.
