# Projects and Scenes

This page covers the first editor loop: create a project, create or open scenes, save work, and set
which scenes are part of the playable build.

## Create or Open a Project

Launch `Ember-Forge` after building the solution. If no project is active, the startup popup offers
two choices:

- **New Project** - enter a project name and a location. The editor creates a project folder and a
  `.ebproj` file.
- **Open Project** - select an existing `.ebproj` file.

You can also use the main menu:

| Action | Menu | Shortcut |
| --- | --- | --- |
| New project | `File > New Project` | `Ctrl+Shift+N` |
| Open project | `File > Open Project` | `Ctrl+Shift+O` |
| Save project | `File > Save Project` | `Ctrl+Shift+S` |

**Save Project** writes everything the project owns in one go: every loaded scene, all materials,
physics materials and animations, the asset registry, and the project settings file.

## Project Folder Layout

New projects use this default layout inside the project folder:

```text
GameData/
  Assets/
    Audio/
    Fonts/
    Materials/
    Models/
    Physics Materials/
    Prefabs/
    Scripts/
    Shaders/
    Textures/
    Assets.eba
  Scenes/
    Default.ebs
```

`Assets.eba` is the asset registry. It stores asset UUIDs and metadata, while the actual source files
live in the asset subfolders. Scene files use the `.ebs` extension and live under `GameData/Scenes`.

## Create, Open, and Save Scenes

Use the `File` menu for scene operations:

| Action | Menu | Shortcut |
| --- | --- | --- |
| New scene | `File > New Scene` | `Ctrl+N` |
| Open scene | `File > Open Scene` | `Ctrl+O` |
| Save scene | `File > Save Scene` | `Ctrl+S` |
| Save scene as | `File > Save Scene As` | — |

`Save Scene` writes only the scene in the active tab (plus the asset registry). To also flush
material, physics material and animation edits, use `File > Save Project` (`Ctrl+Shift+S`).

Save early, especially before playtesting or exporting. The editor performs a full project save
automatically when exporting or launching a standalone runtime session.

## Scenes in Build

Open `Project > Project Settings`, then use the **General** page to manage **Scenes In Build**. This
list controls the scene order used by scripting calls such as:

```lua
SceneManager.LoadDefaultScene()
SceneManager.LoadNextScene()
SceneManager.IsLastScene()
```

The first entry in **Scenes In Build** becomes the project's start scene.

## Project Settings

The Project Settings dialog has four categories:

- **General** - project name and Scenes In Build.
- **Input** - named input actions and the keys, mouse controls and gamepad bindings that trigger
  them, plus a **Devices** section for per-stick and per-trigger deadzone, saturation, response
  curve, actuation and inversion.
- **Physics** - gravity, solver iteration counts, and collision category names.
- **Rendering** - render layer names.

Collision and render layer names become easier to use once named here because component UIs and
Lua filter APIs can show meaningful labels instead of anonymous layer slots.