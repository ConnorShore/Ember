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

Gizmo shortcuts are available in edit mode:

| Key | Gizmo |
| --- | --- |
| `Q` | Disable gizmo. |
| `W` | Translate. |
| `E` | Rotate. |
| `R` | Scale. |
| `T` | Universal transform. |

## Scene Hierarchy

The Scene Hierarchy lists scene entities and their parent-child relationships.

- Click **Create Entity** for an empty entity.
- Right-click the hierarchy background, or press `Space` while the hierarchy has focus, to open the
  creation menu.
- The creation menu includes primitives, lights, cameras, controllers, AI helpers, and VFX helpers.
- Drag entities in the tree to reparent them.
- Right-click an entity to rename, add a child, duplicate, create a prefab, delete, or remove its parent.

Useful entity shortcuts:

| Action | Shortcut |
| --- | --- |
| Rename selected entity | `F2` from the entity context menu. |
| Duplicate selected entity | `Ctrl+D` from the entity context menu. |
| Delete selected entity | `Delete`. |

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

## Debug Windows

Use `Editor > Tool Windows > Render Stats` for renderer information. Use `Editor > Debug` to draw
physics colliders, contact points, AABBs, and AI paths while editing or playtesting.