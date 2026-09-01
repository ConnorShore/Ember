# Scripting Gameplay

Ember gameplay scripts are Lua files attached to entities through a `Script Component`. Script fields
become Inspector properties, and script methods receive lifecycle callbacks during Play mode.

For the complete binding reference, see the [Scripting API Reference](../ScriptingAPI.md).

## Create a Script from the Inspector

1. Select an entity.
2. In the Inspector, use **Add Component > Scripting > Script Component** if the entity does not
   already have one.
3. In the Script Component, click **Create New**.
4. Enter a script name. Ember creates a `.lua` file under `GameData/Assets/Scripts`, registers it as
   an asset, assigns it to the component, and opens it for editing.

You can also right-click in the Asset Manager and use `Import Asset > Script` to import an existing
`.lua` file.

## Script Shape

Every script returns a table. Primitive fields on that table are exposed in the Inspector.

```lua
local Spinner = {}

Spinner.SpeedDeg = 90.0
Spinner.AxisY = true

function Spinner:OnCreate(entity)
    self.Transform = entity:GetComponent("TransformComponent")
end

function Spinner:OnUpdate(entity, delta)
    if self.AxisY then
        self.Transform.Rotation.y = self.Transform.Rotation.y + Math.Radians(self.SpeedDeg) * delta
    end
end

return Spinner
```

After changing exposed fields in a script, use the **Refresh** button in the Script Component to
re-parse the file and update the Inspector property list.

## Common Lifecycle Hooks

| Hook | Use |
| --- | --- |
| `OnCreate(entity)` | Cache components and initialize per-entity state. |
| `OnUpdate(entity, delta)` | Read input, move entities, run gameplay, and update timers. |
| `OnOverlapTriggerEnter(entity, other)` | React when a trigger collider starts overlapping another entity. |
| `OnOverlapTriggerStay(entity, other)` | React while a trigger overlap continues. |
| `OnOverlapTriggerExit(entity, other)` | React when a trigger overlap ends. |

Cross-entity script lookups can be lazy during startup. If `other:GetScriptInstance()` returns `nil`
in `OnCreate`, retry from `OnUpdate` before using the other script.

## Reading Input and Moving an Entity

```lua
local Mover = {}

Mover.Speed = 4.0

function Mover:OnCreate(entity)
    self.Transform = entity:GetComponent("TransformComponent")
end

function Mover:OnUpdate(entity, delta)
    local move = Vector3f.new(0, 0, 0)

    if Input.IsKeyDown(KeyCode.W) then move.z = move.z - 1 end
    if Input.IsKeyDown(KeyCode.S) then move.z = move.z + 1 end
    if Input.IsKeyDown(KeyCode.A) then move.x = move.x - 1 end
    if Input.IsKeyDown(KeyCode.D) then move.x = move.x + 1 end

    if Math.Length(move) > 0 then
        move = Math.Normalize(move)
    end

    self.Transform.Position = self.Transform.Position + move * self.Speed * delta
end

return Mover
```

## Using Assets from Scripts

Fetch assets by type and name, then assign their UUIDs to component handle fields.

```lua
local texture = AssetManager.GetAsset("Texture", "Crosshair")
local sprite = entity:GetComponent("SpriteComponent")
sprite.TextureHandle = texture:GetUUID()
```

Use the asset name shown in the Asset Manager, without the file extension.

## Saving Game Data

`GameData:Open(name)` returns a persistent key/value file for progress or settings. Any number of
files can be open at once, each with its own keys, and the value `Open` returns is safe to keep on
`self`:

```lua
function Player:OnCreate(entity)
    self.progress = GameData:Open("Progress")
    self.settings = GameData:Open("Settings")

    self.volume = self.settings:GetFloat("Volume", 1.0)
end

function Player:OnCheckpoint()
    self.progress:SetInt("Checkpoint", self.progress:GetInt("Checkpoint") + 1)
    self.progress:Save()
end
```

See [Save Game](../ScriptingAPI.md#save-game) for the full list of methods.