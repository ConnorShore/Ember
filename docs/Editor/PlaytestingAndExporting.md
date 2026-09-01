# Playtesting and Exporting

Use Play mode for quick iteration, the standalone runtime button for a closer shipping test, and
Export Project when you need a distributable project folder.

## Play in the Editor

The center toolbar controls editor runtime state:

| Control | Use |
| --- | --- |
| Play | Starts the active scene in Play mode. |
| Stop | Stops Play or Pause mode and returns to Edit mode. |
| Pause | Pauses Play mode; press again to resume. |

`Ctrl+Enter` starts Play mode from Edit mode and stops it from Play mode.

Play mode captures the cursor and the input for the game. **`Shift+F1`** hands both back to the
editor without pausing, so you can inspect panels while the game keeps running; clicking the viewport
takes them back. While the editor holds input the game keeps ticking but reads nothing — mouse look
does not follow the pointer across the editor, and typing in a panel does not drive the player. A key
still held when you click back in reads as held, not as a fresh press.

The chord is deliberately not `Escape`, so games keep `Escape` for their own menus: the editor never
sees that press, and the game never sees `Shift+F1`. Releasing input is separate from the cursor
mode, so a script calling `Input.SetCursorMode(CursorMode.Normal)` for its own menu gets the editor's
mouse back without losing gameplay input.

When Play mode starts, Ember creates a runtime scene copy and fresh Lua state. When you stop, the
editor returns to the edit scene. Make structural scene edits in Edit mode, then press Play again to
test them.

## Standalone Runtime Test

The orange play button saves the active scene and project, then launches `Ember-Runtime` with the
active project path, engine asset directory, and project asset directory. Use this when you want to
test the game outside the editor UI.

If the runtime does not launch, confirm `Ember-Runtime` was built for the current configuration
(`Debug`, `Release`, or `Dist`).

## Export a Project

Use `File > Export Project` or `Ctrl+Shift+E`.

The editor saves the project, asks for an export directory, and copies the project's runtime data
there. The export includes project assets, scenes, and the engine assets needed by the runtime.

Before exporting:

1. Save the active scene.
2. Open `Project > Project Settings > General`.
3. Confirm **Scenes In Build** contains the scenes you want to ship, in load order.
4. Put the intended first scene at index 0.
5. Build the desired runtime configuration in Visual Studio.

## Debugging Playtest Issues

- Empty screen: make sure the scene has an active `Camera Component`.
- Player falls through ground: add colliders to the ground and the player, then enable
  `Editor > Debug > Show Physics Colliders`.
- Scene transition does not advance: confirm **Scenes In Build** has multiple valid scene assets.
- Script properties missing: click **Refresh** in the Script Component after editing the Lua file.
- Runtime stopped unexpectedly: check the log output. The editor catches runtime errors, logs them,
  stops Play mode, and returns to Edit mode so the editor can keep running.