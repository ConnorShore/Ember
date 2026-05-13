# Ember

> A from-scratch C++23 game engine with an editor, runtime, ECS, physics, audio, animation, and Lua scripting.

Ember is a personal game-engine project built from the ground up. It ships with a full
editor (**Ember-Forge**), a standalone runtime (**Ember-Runtime**), and a clean Lua-based
scripting layer so games can be built without recompiling the engine.

---

## Highlights

- **Modern C++23** — built with [premake5](https://premake.github.io/) and Visual Studio 2026.
- **OpenGL renderer** with PBR materials, IBL skyboxes, shadow maps, bloom, FXAA, fog, vignette,
  color-grading LUTs, outlines, billboards, particles, infinite grid, and 2D quad batching.
- **Entity Component System** with a hand-rolled registry, hierarchical transforms, prefabs,
  pooling, and per-component editor UIs.
- **ReactPhysics3D** integration — rigid bodies, box / sphere / capsule / convex / concave colliders,
  raycasts, overlap queries, configurable collision filters, and a kinematic character controller.
- **Skeletal animation** with crossfading, animation events, and a full animator component.
- **Spatial audio** powered by [miniaudio](https://github.com/mackron/miniaudio).
- **AI** — waypoint paths, pathfinding agents, and local avoidance.
- **Lua scripting** via [sol2](https://github.com/ThePhD/sol2) — hot-reloadable per-entity behaviours
  with editor-exposed properties. See [docs/ScriptingAPI.md](docs/ScriptingAPI.md).
- **Asset pipeline** with UUID-based references, a custom `.eba` asset bundle, and a
  drag-and-drop project browser in the editor.
- **Save game system** for persistent key/value storage.
- **Standalone runtime** for shipping built games independent of the editor.

---

## Project Layout

| Folder | Description |
| --- | --- |
| [Ember/](Ember) | The core engine library. All systems, rendering, physics, audio, scripting, ECS. |
| [Ember-Forge/](Ember-Forge) | The editor application. Scene editing, asset management, inspectors, ImGuizmo gizmos. |
| [Ember-Runtime/](Ember-Runtime) | A minimal player application that loads and runs a packaged project. |
| [Ember-Tools/](Ember-Tools) | Offline tooling (asset bundling, etc.). |
| [docs/](docs) | Documentation, including the scripting API reference. |
| [scripts/](scripts) | Helper batch / shell scripts for project generation. |

---

## Building

### Prerequisites

- **Windows 10/11**
- **Visual Studio 2026** (Community works) with the *Desktop development with C++* workload
- A GPU/driver that supports **OpenGL 4.5+**

### Generate the solution

```bat
cd scripts\Windows
GenerateProjects.bat
```

This invokes the bundled `vendor/premake/bin/premake5.exe` with the `vs2026` action. The
generated solution lives in the repository root as `Ember.sln`.

### Build

Open `Ember.sln` in Visual Studio and build. Three configurations are available:

| Configuration | Use for |
| --- | --- |
| **Debug** | Day-to-day development. Asserts and full debug info. |
| **Release** | Optimized build with logging and asserts still enabled. |
| **Dist** | Final shipping configuration. |

`Ember-Forge` is the default startup project. F5 launches the editor.

### Cleaning

```bat
cd scripts\Windows
Clean.bat
```

---

## Quick Start

1. **Build & launch** Ember-Forge.
2. **Create a project** from the project picker (or open an existing one).
3. **Drag assets** (models, textures, audio, animations, shaders) into the Asset Manager panel.
4. **Build a scene** in the viewport. Add entities, attach components, parent them in the hierarchy.
5. **Write gameplay** in Lua. Create a `.lua` file under your project's `scripts/` folder, attach a
   `ScriptComponent` to an entity, and assign the script. Editor-exposed properties from the script
   appear in the inspector automatically.
6. **Press Play**. Iterate. Press Stop. Repeat.

For the complete scripting reference (lifecycle hooks, every component property, math utilities,
input enums, physics queries, audio, save data, and debug drawing), see
[docs/ScriptingAPI.md](docs/ScriptingAPI.md).

---

## A Tiny Lua Script

```lua
local Spinner = {}

Spinner.SpeedDeg = 90.0  -- editable in the inspector

function Spinner:OnUpdate(entity, delta)
    local t = entity:GetComponent("TransformComponent")
    t.Rotation.y = t.Rotation.y + Math.Radians(self.SpeedDeg) * delta
end

return Spinner
```

Drop this onto any entity with a `TransformComponent` and it will spin. The full
[Scripting API reference](docs/ScriptingAPI.md) covers the rest.

---

## Third-Party Libraries

Ember bundles (under `Ember/vendor/` and `Ember-Forge/vendor/`):

- [GLFW](https://www.glfw.org/) — windowing & input
- [glad](https://glad.dav1d.de/) — OpenGL loader
- [glm](https://github.com/g-truc/glm) — math
- [Dear ImGui](https://github.com/ocornut/imgui) — editor UI
- [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) — transform gizmos
- [ReactPhysics3D](https://www.reactphysics3d.com/) — physics
- [miniaudio](https://github.com/mackron/miniaudio) — audio
- [Lua](https://www.lua.org/) + [sol2](https://github.com/ThePhD/sol2) — scripting
- [rapidyaml](https://github.com/biojppm/rapidyaml) — serialization
- [stb](https://github.com/nothings/stb) — image / font loading
- [tinygltf](https://github.com/syoyo/tinygltf) — model importing

---

## License

Released under the [MIT License](LICENSE). You are free to use, modify, and distribute this
project, including for commercial purposes, provided the original copyright notice is retained.
