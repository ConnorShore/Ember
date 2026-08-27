# Editor Tour

Ember-Forge is organized around a few main panels. A normal workflow is: select or create an entity
in the Scene Hierarchy, edit it in the Inspector, drag assets from the Asset Manager, then test with
the toolbar play controls.

## Scene Viewport

The viewport shows the active scene and provides editor camera navigation.

| Input | Result |
| --- | --- |
| Middle mouse drag | Orbit around the focal point. |
| Shift + middle mouse drag | Pan. |
| Mouse wheel | Zoom. |
| Right mouse drag | Rotate the fly camera. |
| Right mouse + `W` `A` `S` `D` | Fly through the scene. |
| Left click entity | Select it, when not dragging a gizmo. |
| `Ctrl` + left click entity | Add it to, or remove it from, the selection. |
| `F` | Frame the selection, including everything under it. |

Gizmo shortcuts are available in edit mode:

| Key | Gizmo |
| --- | --- |
| `Q` | Disable gizmo. |
| `W` | Translate. |
| `E` | Rotate. |
| `R` | Scale. |
| `T` | Universal transform. |

With more than one entity selected, the gizmo pivots on the **active** entity — the one selected last,
and the one the Inspector edits. Moving, rotating or scaling drives the whole selection about that
pivot. A child whose parent is also selected is skipped, since it already moves with its parent.

### Snapping and editor toggles

Left of the play controls:

- **View** snaps the editor camera to an axis, or back to free-fly.
- **Gizmos** opens every editor toggle in one dropdown. The button label shows the current snap
  increments (`Gizmos 1 / 90°`) so the setting you change most is readable without opening it.

The Gizmos dropdown holds:

| Section | Settings |
| --- | --- |
| Snapping | Enable snapping, grid increment, angle increment. |
| Gizmo | World or Local space. |
| Placement | Spawn new entities at the cursor. |
| Display | Draw all HUD icons, or only the selected entity's. |
| Debug Draw | Physics colliders (shapes, contact points, AABBs), AI paths, selected navmesh. |

Snapping is **on by default** at 1 unit and 90°, which suits assembling modular kit pieces. Holding
`Ctrl` while dragging a gizmo *inverts* the toggle, so it still snaps when snapping is switched off
and moves freely when it is on. The debug-draw toggles are also reachable from `Editor > Debug`;
both drive the same settings.

## Scene Hierarchy

The Scene Hierarchy lists scene entities and their parent-child relationships.

- Click **Create Entity** for an empty entity.
- Right-click the hierarchy background, or press `Space` while the hierarchy has focus, to open the
  creation menu.
- The creation menu includes primitives, lights, cameras, controllers, AI helpers, and VFX helpers.
- Drag entities in the tree to reparent them, or between them to reorder siblings.
- Right-click an entity to rename, add a child, duplicate, create a prefab, delete, or remove its parent.
- `Ctrl` + click toggles an entity in the selection; `Shift` + click selects the run between the last
  clicked entity and this one.

Useful entity shortcuts:

| Action | Shortcut |
| --- | --- |
| Rename active entity | `F2` from the entity context menu. |
| Duplicate selection | `Ctrl+D`. The copies stay selected, so it can be repeated. |
| Delete selection | `Delete`. |
| Select all visible entities | `Ctrl+A`. Collapsed subtrees are not included. |

New entities no longer land at the world origin. Dragging a model or prefab into the viewport places
it on whatever surface is under the cursor, snapped to the grid increment when snapping is on.
Creating from the hierarchy menu places it at the camera's focal point instead — the cursor is over
the hierarchy panel, not the viewport — so frame the area you want first. Turn cursor placement off
under **Gizmos > Placement**.

## Undo and Redo

| Action | Shortcut |
| --- | --- |
| Undo | `Ctrl+Z` |
| Redo | `Ctrl+Y` or `Ctrl+Shift+Z` |

`Edit > Undo` names the action that will be reversed. History is **per viewport tab**, so a scene and
a prefab open side by side never share a stack, and closing a tab discards its history. Play mode runs
on a copy of the scene, so nothing done during play enters the history.

A whole gizmo drag, slider drag or typing session collapses into a single step rather than one step
per frame, and an edit that ends where it started records nothing at all.

## Inspector

The Inspector edits the selected entity.

- The checkbox near the entity header toggles the entity active or inactive.
- `Transform Component` is always present. The Inspector displays rotation in degrees, while scripts
  use radians.
- Use **Add Component** to add rendering, lighting, physics, audio, animation, scripting, and AI
  components.
- Component asset fields accept drag-and-drop payloads from the Asset Manager.

## Asset Manager

The Asset Manager shows the active project's asset folders. Right-click empty space for:

- **New Directory**.
- **Import Asset > Model**.
- **Import Asset > Texture**.
- **Import Asset > Shader**.
- **Import Asset > Script**.
- **Import Asset > Prefab**.
- **Import Asset > Font**.
- **Import Asset > Audio**.

Imported files are copied into the current folder when needed and registered in `Assets.eba`.
Right-click a script asset and choose **Edit Script** to open it in the script editor.

## Prefabs

Right-click an entity and choose **Create Prefab From Entity**, or drag it from the hierarchy into the
Asset Manager. Drag a `.ebprefab` into the viewport to place an instance at the cursor, or onto a
hierarchy entity to place it as that entity's child. Double-click one to open it in its own tab.
Prefab-instance roots are drawn in orange in the hierarchy.

### Updating placed instances

Saving a prefab that has instances in an open scene asks whether to update them. Declining leaves the
placed copies exactly as they are.

Updating rebuilds each instance from the prefab. **Kept per instance:**

- Position, rotation and scale
- Name
- Parent and sibling order
- Script property values (`Add Component > Script` properties set on that instance)

**Replaced by the prefab:** everything else — swapped materials or sprites, components added to a
single instance, children added to a single instance.

The update is undoable, but the undo entry belongs to the **scene** tab rather than the prefab tab you
saved from: switch to the scene and press `Ctrl+Z`.

Two limits worth knowing:

- Script property values are re-attached by matching entity names down the subtree. Renaming a child
  *inside* an instance breaks that match, and that child's values fall back to the prefab's.
- An instance placed before its prefab carried a prefab link cannot be found by an update. If a prefab
  was created in an older build, re-place its instances once.

## Environment Panel

The Environment panel controls global rendering settings for the scene, including skybox texture and
ambient intensity, bloom, FXAA, fog, vignette, and color grading. These settings are useful for quick
scene-wide lighting and post-processing adjustments before you create local post-process volumes.

## Toolbar

The center toolbar includes runtime controls:

- Orange play button - save and launch the active project through `Ember-Runtime`.
- Play button - enter Play mode inside the editor.
- Stop button - exit Play or Pause mode and return to Edit mode.
- Pause button - pause or resume Play mode.

`Ctrl+Enter` toggles between starting and stopping Play mode.

The left of the toolbar holds the camera **View** selector and the **Gizmos** dropdown — see
[Snapping and editor toggles](#snapping-and-editor-toggles).

## Debug Windows

Use `Editor > Tool Windows > Render Stats` for renderer information. Use `Editor > Debug` to draw
physics colliders, contact points, AABBs, AI paths, and the selected navmesh while editing or
playtesting. The same toggles are in the toolbar's **Gizmos** dropdown; both drive the same settings.