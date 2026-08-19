# Building UI

How to build menus, HUDs and store screens in Ember-Forge, and wire them to gameplay.

For the script-side API reference (component fields, callbacks, the `UI` table) see
[`docs/ScriptingAPI.md`](../ScriptingAPI.md).

## The model

Ember's UI follows the same shape as Unity's uGUI and Godot's `Control` tree:

- A **Canvas** is the root of a UI tree. Everything under it is laid out in screen space.
- A **Rect Transform** positions an element inside its parent using anchors, a pivot and a size.
- A **UI Selectable** adds the shared interaction state machine — hover, press, focus, disabled,
  plus the visual response. It is what makes an element respond to input at all.
- A **role component** on the same entity says what activating it does: **UI Button** fires a
  click, **UI Toggle** flips an on/off value.

Selectable is deliberately separate from Button and Toggle. Sliders, scroll views and dropdowns
will reuse the same state machine, navigation and focus handling without changes.

## Your first button

1. In the Scene Hierarchy, right-click → **UI → Canvas**.
2. Right-click the Canvas → **UI → Button**.

The preset builds the whole assembly: a parent carrying Rect Transform, Sprite, UI Selectable and
UI Button, plus a **Label** child stretched to fill it. Set the label's text and `Font Size` in the
Inspector.

Press Play and hover it — the background tints, and clicking it darkens further.

### Coordinate space

UI is laid out in **viewport pixels with the origin at the bottom-left and +Y up**. This is the
space `Rect Transform` values are in, and the space `Input.GetViewportMousePosition()` returns.

### Anchors, pivot and size

- **Anchors** are fractions of the *parent* rect: `(0,0)` is its bottom-left, `(1,1)` its top-right.
- When `Anchor Min == Anchor Max`, **Size Delta** is the element's width and height, and
  **Anchored Position** offsets it from the anchor point.
- When they differ, the element stretches between them and Size Delta acts as **padding**. Anchors
  of `(0,0)`–`(1,1)` with zero Size Delta fill the parent exactly — the usual setup for a
  full-screen panel or a label inside a button.
- **Pivot** is the element's own origin, used for both positioning and rotation.

### Resolution independence

The Canvas's **Reference Resolution** is the resolution you author at. At runtime everything scales
by the viewport's ratio to it, blended between the width and height axes by **Match Width Or
Height**. A 200px button stays proportionally 200px at 720p and at 4K.

### Draw order

Elements draw in hierarchy order, depth-first: **later siblings draw on top**, and a child draws
over its parent. Canvases draw in **Sort Order**, low to high — put a pause menu on a canvas with a
higher Sort Order than the HUD.

Hit-testing uses the same order, so whatever you see on top is what you click.

## Wiring a button to gameplay

Two ways, both Lua.

### Lifecycle hook — for static UI

Put a script on the button entity itself and implement `OnClick`. No registration:

```lua
local PlayButton = {}

function PlayButton:OnClick(entity)
    SceneManager.LoadScene("LevelOne")
end

return PlayButton
```

The same works for `OnValueChanged(self, entity, isOn)` on a toggle, and `OnHoverEnter` /
`OnHoverExit` on anything selectable.

### Registered handler — for UI built at runtime

When rows are created dynamically (a store list, an inventory), register handlers as you build
them:

```lua
function Store:OnCreate(entity)
    for _, row in ipairs(entity:GetChildren()) do
        local button = row:GetComponent("UIButtonComponent")
        button:OnClick(function(clicked)
            self:Purchase(clicked:GetName())
        end)
    end
end
```

Registered handlers last for one play session and are dropped when play stops, so register them
from `OnCreate` rather than at file scope.

## Toggles and radio groups

A **UI Toggle** owns a `Checkmark` entity that is shown and hidden as `Is On` changes — the preset
wires one up for you.

For radio behaviour, point several toggles at the same **Group** entity (any entity works as the
key). Turning one on turns the rest off, and clicking the active one does nothing unless **Allow
Switch Off** is ticked.

## Keyboard navigation

Selectables support directional focus. Arrow keys move focus, **Enter** or **Space** activates the
focused element, **Escape** clears focus.

With nothing focused yet, the first arrow press adopts the first selectable in draw order, so a
keyboard-only player can reach a menu without clicking it first. A focused element shows its
**Selected Color**.

- **Automatic** navigation picks the best-aligned nearby selectable in the direction pressed. This
  is usually all you need.
- **Explicit** navigation lets you name the target for each direction, for layouts where the
  automatic choice is wrong.
- **None** removes the element from navigation entirely.

## Blocking clicks that land on the HUD

Gameplay should ignore a click that hit the UI:

```lua
function Weapon:OnUpdate(entity, delta)
    if Input.IsMouseButtonPressed(MouseButton.Left) and not UI.IsPointerOverUI() then
        self:Fire()
    end
end
```

An element counts as blocking if it has a UI Selectable, or a Rect Transform with **Raycast
Target** ticked. Raycast Target is off by default, so decorative art does not swallow clicks. Tick
it on a full-screen rect to make a modal backdrop.

## The cursor

Both the editor's Play mode and the standalone runtime **lock the cursor on start** for
first-person play. A locked cursor reports unbounded virtual coordinates, so **UI is not clickable
while it is locked**.

For a menu, unlock it:

```lua
function MainMenu:OnCreate(entity)
    Input.SetCursorMode(CursorMode.Normal)
end
```

In the editor, clicking the viewport re-locks the cursor — except when the click landed on UI, so
buttons keep working once you have unlocked.

## Text

`Font Size` is the pixel height of the text and is independent of the element's rect, so resizing a
button does not resize its caption. **Horizontal Align** and **Vertical Align** position the text
block inside the rect; `Center` / `Center` centres a caption in a button.

## Gotchas

- **Interaction only runs in Play mode.** In Edit mode the UI lays out and renders but does not
  respond, matching Unity. This is also what keeps hover tinting from dirtying your saved scene.
- **Disabling a panel hides its whole subtree**, including enabled children.
- **The Canvas entity itself is not drawn** — it is a container. Put your background on a child.
- **Tint colours are multiplied** by the target graphic's authored colour rather than replacing it,
  so a white button art asset takes the tint directly while a coloured one keeps its hue.
