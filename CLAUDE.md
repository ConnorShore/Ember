# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Ember is a from-scratch C++23 game engine (Windows/OpenGL) with an editor (**Ember-Forge**), a
standalone player (**Ember-Runtime**), offline asset tooling (**Ember-Tools**), and a Lua scripting
layer (sol2) so games can be built without recompiling the engine. See `README.md` for the feature
list and `docs/ScriptingAPI.md` / `docs/ShaderAPI.md` for the scripting and shader authoring
references consumed by game authors (not engine code).

## Build System

Premake5-generated Visual Studio solution — there is no CMake, no package manager, and no CLI build
command that doesn't go through Visual Studio.

```bat
cd scripts\Windows
GenerateProjects.bat   REM cleans, then runs vendor\premake\bin\premake5.exe vs2026 to (re)generate Ember.sln
Clean.bat               REM removes generated sln/vcxproj files, .vs/, bin/, bin-int/, and vendor build dirs
Package.bat              REM zips a Dist build of Ember-Forge + Ember-Runtime for distribution
```

Regenerate projects (`GenerateProjects.bat`) any time a `premake5.lua` file changes or new source
files/vendor libs are added — `.vcxproj` file lists are not auto-synced with the filesystem.

Build via Visual Studio 2026 (Desktop development with C++ workload) by opening `Ember.slnx` /
`Ember.sln` and building. `Ember-Forge` is the startup project (F5 launches the editor). Three
configurations: **Debug** (asserts + full debug info + profiling), **Release** (optimized, logging/
asserts still on), **Dist** (final shipping build, no symbols). There is a fourth "Profile" filter
defined in each `premake5.lua` (optimized + profiling + symbols) for performance investigations, but
it is not currently listed in the top-level `workspace` configurations in `premake5.lua`.

Validate changes with the `Ember-Test` suite (see **Testing** below) and, for anything user-facing,
by exercising the feature in Ember-Forge (or Ember-Runtime for a packaged project) — see
`docs/Editor/PlaytestingAndExporting.md`.

### Submodules

Most third-party libraries live under `Ember/vendor/`, `Ember-Forge/vendor/`, and
`Ember-Tools/vendor/` as **git submodules** pointed at `ConnorShore/*` forks (see `.gitmodules`).
Run `git submodule update --init --recursive` after cloning; an uninitialized submodule shows as an
empty/untracked directory and premake/MSBuild will fail to find its headers.

## High-Level Architecture

### The five projects

| Project | Kind | Depends on | Role |
|---|---|---|---|
| `Ember` | StaticLib | (vendor only) | The engine: ECS, rendering, physics, audio, animation, scripting, asset pipeline. Everything else links against this. |
| `Ember-Runtime` | — | `Ember` | Minimal player app that loads and runs a packaged project (`RuntimeLayer`). No editor UI. |
| `Ember-Tools` | — | `Ember` | Offline tooling used by the editor at edit-time — currently glTF import (`GLTFImporter`). |
| `Ember-Forge` | ConsoleApp | `Ember`, `Ember-Runtime`, `Ember-Tools`, ImGuizmo, imgui-node-editor | The editor. Scene/asset authoring, inspectors, gizmos; embeds the runtime for Play mode. |
| `Ember-Test` | ConsoleApp | `Ember` | The test suite. Boots a minimal `Application` (window + GL context + default assets + a throwaway `Project`), runs every registered test on frame one, exits 0/1. See Testing below. |

Each project has its own precompiled header (`ebpch`/`efpch`/...) and its own `premake5.lua`; include
paths reach across projects via relative `%{wks.location}/...` paths rather than a shared public
include root, so cross-project includes look like
`"%{wks.location}/Ember/src"` from `Ember-Forge`'s premake file.

### Application / Layer model (`Ember/src/Ember/Core`)

`Application` (one process-wide instance, `Application::Instance()`) owns the `Window`, a
`LayerStack`, `SystemManager`, `AssetManager`, `SceneManager`, and `SaveGameManager`. `Layer`s are
pushed onto the stack (`PushLayer` / `PushCanvasLayer`) and receive `OnAttach`/`OnDetach`/`OnUpdate`/
`OnEvent`. `Ember-Forge` adds `EditorLayer`; `Ember-Runtime` adds `RuntimeLayer`. `EntryPoint.h`
wraps `main()` and brackets Startup/Runtime/Shutdown in profiler sessions (see Profiling below); each
executable implements `Ember::CreateApplication(argc, argv)` to construct its `Application` subclass.

### ECS (`Ember/src/Ember/ECS`)

A **hand-rolled, non-entt** ECS — not an existing library:

- `EntityManager` — entity IDs and which `ComponentType`s each entity has, plus per-entity component
  ordering (`GetComponentOrder`/`SetComponentOrder`, used by the editor's inspector).
- `ComponentManager` — packed per-type component storage and active-entity lists.
- `Registry` — the façade over both. `AttachComponent<T>`/`DetachComponent<T>`/`GetComponent<T>`,
  plus a component lifecycle system: `OnComponentAttached<T>()` / `OnComponentDetached<T>()` return a
  `ComponentLifecycleRegistry<T>` you `Connect()` a callback to; `ConnectAndRetroact<T>` connects a
  callback *and* immediately fires it for every existing component of that type (used to lazily wire
  up GPU/physics resources when a component is added to an already-populated scene).
- `View`/`Query` — `registry.Query<Driver, ComponentA, ComponentB>()` and
  `registry.ActiveQuery<Driver, ...>()` (the latter auto-excludes `DisabledComponent`) return an
  iterable view over entities with the requested components; `Query<Driver, Args...>(Exclude<X>{})`
  supports explicit exclusion sets.
- `System` — base class with `OnAttach`/`OnDetach`/`OnSceneAttach`/`OnSceneDetach`/
  `OnUpdate(TimeStep, Scene*)`. Concrete systems live in `ECS/System/` (`RenderSystem`,
  `PhysicsSystem`, `AnimationSystem`, `ScriptSystem`, `AudioSystem`, `AISystem`,
  `CharacterControllerSystem`, `TransformSystem`, `ParticleSystem`, `BoneSocketSystem`,
  `UILayoutSystem`, `LifecycleSystem`) and are owned/driven by `SystemManager`.

`Scene` (`Ember/src/Ember/Scene`) wraps a `Registry` + `PoolManager` and is itself an `Asset`. It owns
entity lifecycle (`AddEntity`, `DuplicateEntity`, `RemoveEntity`, prefab instantiation), parenting/
hierarchy (`SetEntityParent`, `ReorderEntity`), and Edit vs. Play/Pause state
(`OnRuntimeStart`/`OnRuntimeStop`, `OnUpdateEdit` vs `OnUpdateRuntime`). `Entity` is a thin
`(EntityID, Scene*)` handle, not an owning object — copies are cheap and interchangeable.
`SceneManager` handles which `Scene` is currently active/loading.

### Assets (`Ember/src/Ember/Asset`)

Every loadable resource derives from `Asset` and is identified by a `UUID`. `AssetManager` is a
single generic store keyed by `UUID` (with secondary name/path lookup maps) exposing
`Create<T>`/`Load<T>`/`Register<T>`/`GetAsset<T>`/`SaveAssetToFile<T>`; the type-specific load/save
logic is dispatched with `if constexpr` chains against `Serializers/*Serializer` classes (YAML via
the `rapidyaml` submodule) or importer classes (`TextureImporter`, `ShaderImporter`,
`ScriptImporter`). Loads are de-duplicated by absolute file path. `AssetManager::PollShaderHotReload`
watches non-engine shader files' mtimes and reloads changed ones — called once per frame from the
editor. Packaged/shipped projects bundle assets into a custom `.eba` bundle (see README) built by
`Ember-Tools`.

### Scripting (`Ember/src/Ember/Script`)

Gameplay scripting is Lua via **sol2**. `ScriptEngine` owns the global `sol::state`, binds the engine
API (`BindAPI`), and drives per-entity script lifecycle through `ScriptSystem`. Engine-side bindings
live one-per-domain under `Script/Bindings/` (`ScriptBindEntity`, `ScriptBindComponents*`,
`ScriptBindPhysics`, `ScriptBindInput`, `ScriptBindAudio`, `ScriptBindMath`, `ScriptBindScene`,
`ScriptBindSaveGame`, `ScriptBindDebugDraw`, ...) — when exposing a new engine feature to Lua, add a
binding function in the matching file (or a new one) and register it from `ScriptBindCore`/`BindAPI`
rather than inlining sol2 calls elsewhere. Scripts are hot-reloadable per-entity behaviors
(`ScriptComponent`) with editor-exposed properties (`ScriptProperty`); the lifecycle hook names Lua
scripts implement are enumerated in `ScriptEngine::DefaultEmberFunctions` (`OnCreate`, `OnUpdate`,
`OnOverlapTriggerEnter/Stay/Exit`). Full authoring-facing API surface is documented in
`docs/ScriptingAPI.md` — keep it in sync when bindings change since it's the primary reference for
game-side (not engine-side) scripting.

### Rendering

OpenGL 4.5+ forward-ish pipeline organized as render passes under `Ember/src/Ember/Render/Pass/`
(shadow, opaque, post-process, billboards, etc.), driven by `RenderSystem`. Platform GL objects live
under `Ember/src/Ember/Platform/OpenGL`. VFX/post settings (bloom, fog, vignette, color grading LUTs)
are under `Render/VFX`.

### Physics

Wraps **ReactPhysics3D** (vendored submodule) via `PhysicsSystem` and `Ember/src/Ember/Physics/`
(collision/overlap callbacks, event listener). `CharacterControllerSystem` implements the kinematic
character controller on top of it.

### Editor (`Ember-Forge`)

`EditorLayer` is the central editor controller; UI is organized into `Panels/` (Scene Hierarchy,
Asset Manager, Environment, Animation Scrubber, Notifications, ...), `ComponentUI/` (per-component
inspector drawers, mirroring `Ember/ECS/Component/Components.h`), `GraphNodeUI/` (node-editor UI, e.g.
for animation graphs, via the vendored `imgui-node-editor`), and `Viewers/`. Gizmos use the vendored
`ImGuizmo`. `EditorContext.h` / `EditorConstants.h` hold shared editor-wide state/constants. The
editor embeds `Ember-Runtime`'s systems to implement Play mode inside the viewport rather than
shelling out to a separate process.

### Profiling

Two independent, purpose-built systems (not Tracy/Optick, despite `docs/ProfilingAndBenchmarking.md`
describing a Tracy-based design that was **not** what got implemented):

- **Chrome-tracing instrumentor** (`Ember/src/Ember/Performance/Instrumentor.h` + `Profiler.h`) —
  `EB_PROFILE_SCOPE(name)` / `EB_PROFILE_FUNCTION()` RAII timers write Chrome `chrome://tracing`
  /Perfetto-format JSON. Sessions are compiled out entirely unless `EB_PROFILE` is defined (on in
  Debug and the unused-by-default Profile config). `EntryPoint.h` brackets three sessions per run —
  `Profiles/Startup.json`, `Profiles/Runtime.json`, `Profiles/Shutdown.json` — relative to the
  working directory (`debugdir` is the workspace root).
- View captured traces with `scripts\Windows\LaunchProfilerViewer.bat`, which opens each
  `Profiles/*.json` in the Perfetto UI (https://ui.perfetto.dev/) via the vendored
  `scripts/vendor/open_trace_in_ui.py` helper (requires Python 3).

`docs/ProfilingAndBenchmarking.md` is still useful for the *methodology* (ms vs FPS, budget tables,
golden-scene benchmarking, per-system suspect triage for linear per-entity cost) — just substitute
the actual `EB_PROFILE_SCOPE`/Perfetto stack above wherever it references Tracy or a hypothetical
`FrameProfiler`/`ScopedTimer` API.

## Testing

`Ember-Test` is the automated suite (213 tests as of this writing). **`Ember-Test/README.md` is the
reference** — read it before writing tests. It documents the assertion macros, the scene fixtures,
the golden-image workflow, and a "Traps worth knowing about" list of engine footguns that have
already produced wrong tests (component references invalidated by attach,
`static_cast<bool>(entity)` being inverted, the physics camera sensor parked at the world origin,
and others).

```bat
scripts\Windows\RunTests.bat                      REM Release, everything
scripts\Windows\RunTests.bat Debug                REM Debug (auto-sets EMBER_TEST_PERF_SCALE=8)
scripts\Windows\RunTests.bat --filter=unit        REM one category: unit|integration|visual|performance|stress
scripts\Windows\RunTests.bat --run=Physics        REM only tests whose Suite::Name contains "Physics"
```

Exit code is 0 (all passed) or 1 (any failure). `Logs/test-progress.log` records every test with its
failure messages and notes, flushed **before** each test runs — so if a test hard-crashes the process
(in Debug an `EB_CORE_ASSERT` is `__debugbreak()`), the last `RUNNING` line names the culprit.

### Run the suite after any large change

After a non-trivial change to the engine — a system, the ECS, serialization, the asset pipeline,
physics, scripting bindings, rendering — verify it against the suite before calling the work done.

**Claude cannot build this project.** There is no CLI build; it requires Visual Studio. So:

1. Ask the user to rebuild `Ember-Test` (Release, and Debug too if the change could be
   optimization-sensitive — several bugs here have been Release-only).
2. Then run `scripts\Windows\RunTests.bat` and read the results.
3. Report failures honestly, with the assertion text from `Logs/test-progress.log`.

**Never run `RunTests.bat` against a binary that predates your edits** — it will happily pass and
prove nothing. If you are unsure whether a rebuild has happened since your changes, ask.

When a test fails, work out whether the *test* encodes a wrong assumption or the *engine* has a real
defect, and say which. Several failures in this suite have been genuine engine bugs (see the
"Recently fixed (found by Ember-Test)" section of `TODOs.md`); do not "fix" a test by weakening it
into agreement with buggy behaviour.

### Add tests alongside large changes

When adding a feature or fixing a non-trivial bug, add coverage in the same change:

- **New engine subsystem or system** → a new `Ember-Test/src/Tests/<Area>Tests.cpp`, following the
  existing per-subsystem layout.
- **New behaviour in an existing area** → extend that area's file.
- **Bug fix** → a regression test that fails before the fix and passes after, with a comment naming
  the defect. `Scene::DuplicateEntityCopiesChildren` and `Scene::AttachmentChildIgnoresParentScale`
  are the model.
- **Performance-sensitive work** → a budget in `PerfTests.cpp`. Prefer *relative* assertions (X must
  be cheaper than Y) over absolute millisecond budgets where possible: they are machine-independent
  and are the only way to prove an optimisation still engages. `Perf::TransformDirtyFlagActuallySkipsWork`
  is the model.
- **Rendering changes** → the frame-sanity and determinism checks in `VisualTests.cpp` need no golden
  image and catch gross corruption on any machine. Only mint a new golden (`EMBER_TEST_WRITE_GOLDEN=1`)
  from a build you have verified looks correct.

Adding a test file means re-running `scripts\Windows\GenerateProjects.bat` — premake globs
`src/**.cpp`, but the generated `.vcxproj` file list is not auto-synced with the filesystem.

Not everything warrants a test: pure editor-UI layout, one-line comment or logging changes, and
anything needing assets that are not in the repo are reasonable to skip. Say so rather than
inventing a test that asserts nothing.

## Conventions

- **Comments**: default to a single line and a single sentence — a brief note on what the following
  code does, or why it does it. Favour *why* when the code already says *what*. Wrap onto a second
  line only when one sentence genuinely doesn't fit; a second sentence should be rare, and
  multi-paragraph block comments are never right — if something needs that much context it belongs
  in a README or `docs/`, not inline. This applies to test files too.
- **Naming**: `EB_` prefix for engine macros (`EB_CORE_INFO/WARN/ERROR/FATAL` for engine-side
  logging, `EB_INFO/WARN/...` for app-side, `EB_CORE_ASSERT`, `EB_PROFILE_*`). `m_` prefix for member
  variables, `s_` for statics.
- **Smart pointers**: engine-wide `ScopedPtr<T>` (unique-ownership) and `SharedPtr<T>` /
  `WeakPtr<T>` wrappers (`Ember/src/Ember/Core/ScopedPointer.h`, `SharedPointer.h`) are used instead
  of `std::unique_ptr`/`std::shared_ptr` directly — use `StaticPointerCast`/`DynamicPointerCast`
  rather than `std::static_pointer_cast`/`dynamic_pointer_cast`.
- Build configs gate behavior via defines: `EB_DEBUG`, `EB_RELEASE`, `EB_DIST`, `EB_PROFILE`,
  `EB_EDITOR` (Ember-Forge only), `EB_ENGINE` (Ember only) — check these rather than assuming a given
  feature (asserts, profiling, editor-only code paths) is always compiled in.
- `TODOs.md` is a maintained, prioritized triage list of in-code `// TODO:` comments across all five
  projects (P0 blockers through P3 post-MVP). Check it before assuming a rough edge is unknown, and
  update it if you resolve or add a significant TODO.
