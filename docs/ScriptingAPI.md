# Ember Scripting API Reference

Ember uses **Lua 5.4** (via [sol2](https://github.com/ThePhD/sol2)) as its embedded scripting language.
Every gameplay script is a Lua file that returns a single table whose fields become the script's
properties (exposed in the editor) and whose methods are invoked by the engine at runtime.

This document is the canonical reference for the entire scripting API exposed to user scripts. It
mirrors the bindings registered in [`Ember/src/Ember/Script/Bindings/`](../Ember/src/Ember/Script/Bindings).

---

## Table of Contents

- [Script Structure &amp; Lifecycle](#script-structure--lifecycle)
  - [Enum Properties](#enum-properties)
  - [Reference Properties](#reference-properties)
  - [Reference Arrays](#reference-arrays)
  - [Script Inheritance (`Base`)](#script-inheritance-base)
- [Core Types](#core-types)
  - [`UUID`](#uuid)
  - [`TimeStep`](#timestep)
  - [`Time` / `Timer`](#time--timer)
    - [`Log`](#log)
  - [`Window` / `Renderer`](#window--renderer)
- [Math](#math)
  - [Vectors &amp; Matrices](#vectors--matrices)
  - [`Quaternion`](#quaternion)
  - [`Math` table](#math-table)
- [Entity &amp; Scene](#entity--scene)
  - [`Entity`](#entity)
  - [`Scene`](#scene)
  - [`SceneManager`](#scenemanager)
- [Components](#components)
  - [`TransformComponent`](#transformcomponent)
  - [Rendering Components](#rendering-components)
  - [Lighting Components](#lighting-components)
  - [`CameraComponent`](#cameracomponent)
  - [Physics Components](#physics-components)
  - [Audio Components](#audio-components)
  - [AI Components](#ai-components)
  - [Misc Components](#misc-components)
- [Input](#input)
- [Physics](#physics)
- [Audio](#audio)
- [Assets](#assets)
- [Particles](#particles)
- [Save Game](#save-game)
- [Debug Drawing](#debug-drawing)
- [Standard Lua Libraries](#standard-lua-libraries)

---

## Script Structure & Lifecycle

Every Ember script is a Lua module that returns a table. Fields on this table that match a
supported type (`int`, `float`, `string`, `bool`, `Vector3f`, an enum table — see
[Enum Properties](#enum-properties) — or an entity/asset reference — see
[Reference Properties](#reference-properties)) are surfaced as **editable properties** in the
Inspector. Any of the following methods, if present, are invoked by the engine:

| Method                                         | When called                                                    | Signature                                   |
| ---------------------------------------------- | -------------------------------------------------------------- | ------------------------------------------- |
| `OnCreate(self, entity)`                     | Once, when the script is first instantiated.                   | `entity: Entity`                          |
| `OnUpdate(self, entity, delta)`              | Every frame the entity is active.                              | `entity: Entity, delta: number` (seconds) |
| `OnOverlapTriggerEnter(self, entity, other)` | A trigger collider on`entity` started overlapping `other`. | `entity, other: Entity`                   |
| `OnOverlapTriggerStay(self, entity, other)`  | Continuous overlap with`other`.                              | `entity, other: Entity`                   |
| `OnOverlapTriggerExit(self, entity, other)`  | The overlap with`other` ended.                               | `entity, other: Entity`                   |
| `OnAnimationEvent(self, eventName)`          | An animation clip fired a named event.                         | `eventName: string`                       |
| `OnClick(self, entity)`                      | A UI button on`entity` was activated.                        | `entity: Entity`                          |
| `OnValueChanged(self, entity, isOn)`         | A UI toggle on`entity` changed state.                        | `entity: Entity, isOn: bool`              |
| `OnHoverEnter(self, entity)`                 | The pointer entered a UI selectable on`entity`.              | `entity: Entity`                          |
| `OnHoverExit(self, entity)`                  | The pointer left a UI selectable on`entity`.                 | `entity: Entity`                          |

> The engine treats those names as reserved lifecycle hooks. Any other field on the returned
> table is considered a *property* and is candidate for editor exposure.

### Minimal script template

```lua
local MyScript = {}

-- Editor-exposed properties
MyScript.Speed       = 5.0
MyScript.Health      = 100
MyScript.PlayerName  = "Hero"
MyScript.IsInvincible = false

function MyScript:OnCreate(entity)
    -- Cache state on `self` here
end

function MyScript:OnUpdate(entity, delta)
    -- delta is seconds since last frame
end

return MyScript
```

> **Important:** Always `return` the table at the bottom of the file. The returned table is treated
> as the *class* — each entity that uses the script gets its own instance of it.

### Accessing other scripts on an entity

```lua
local other = entity:GetScriptInstance() -- returns the script table on this entity, or nil

-- Optionally filter by name - matches either the entity's concrete script or any script in its
-- Base ancestry (see Script Inheritance below), so this also matches an entity whose attached
-- script is PickupItem if PickupItem.Base = "PurchasableItem":
local purchasable = entity:GetScriptInstance("PurchasableItem") -- nil if neither matches
```

Script instances are created lazily while scripts update. If one script needs another entity's
script instance during startup, resolve it in `OnUpdate` or retry until `GetScriptInstance()`
returns a table instead of assuming it is ready inside `OnCreate`.

### Enum Properties

A top-level table whose keys are strings and values are integers is exposed as a **combo box**
in the inspector. The script can use the table like a regular enum at runtime.

```lua
local Pickup = {}

Pickup.Kind = {
    Ammo   = 1,
    Health = 2,
    Points = 3,
}

function Pickup:OnUpdate(entity, delta)
    if self.Kind == Pickup.Kind.Health then
        -- ...
    end
end

return Pickup
```

The option list is taken from the table at script-load time and the *first option* (lowest int)
is the default. The editor stores the selected option's integer value on the component, so reading
`self.Kind` from Lua returns a plain number — compare it against the named entries on the
original table.

### Reference Properties

A property can hold a reference to a scene entity or to an asset. Declare it with one of the
reference constructors and assign the actual target in the Inspector — by dragging an entity from
the Scene Hierarchy or an asset from the Asset Manager onto the property's slot.

```lua
local Turret = {}

Turret.Target        = EntityRef()               -- an entity in the scene
Turret.ProjectilePrefab = PrefabRef()            -- a .ebprefab asset
Turret.FireSound     = AudioClipRef()            -- an audio clip
Turret.MuzzleFlash   = AssetRef("Texture")       -- any asset kind, by name

return Turret
```

`AssetRef(kind)` accepts `"Texture"`, `"Mesh"`, `"Model"`, `"Script"`, `"Shader"`, `"Material"`,
`"PhysicsMaterial"`, `"Prefab"`, `"Font"`, `"AudioClip"` and `"Scene"`. The slot only accepts drops
of the declared kind.

At runtime the property reads back as a `UUID`, which is what the reference-taking APIs expect:

```lua
function Turret:Fire()
    if self.ProjectilePrefab:IsValid() then
        Scene.InstantiatePrefab(self.ProjectilePrefab, self.SpawnPoint)
    end
end
```

An unassigned reference is an invalid `UUID` — always guard with `:IsValid()` before using one.

### Reference Arrays

To let a designer build a *list* of references without editing the script — a pool of spawnable
prefabs, a set of patrol points, a bank of hit sounds — declare a reference array. The Inspector
draws it as a resizable list: **+** adds a slot, **X** removes one, and each slot is a drop target.

```lua
local PowerUpManager = {}

PowerUpManager.PowerUpPrefabs = PrefabRefArray()
PowerUpManager.DeathSounds    = AudioClipRefArray()
PowerUpManager.PatrolPoints   = EntityRefArray()
PowerUpManager.Decals         = AssetRefArray("Texture")

return PowerUpManager
```

The declaration is always empty — the contents live on the component, so two entities using the same
script can each have their own list.

In Lua the property is a plain **1-based table** of `UUID`s, so `#`, `ipairs` and indexing all work:

```lua
function PowerUpManager:OnCreate(entity)
    -- Empty Inspector slots come through as invalid UUIDs; filter them out once up front
    self.Pool = {}
    for _, prefab in ipairs(self.PowerUpPrefabs) do
        if prefab:IsValid() then
            table.insert(self.Pool, prefab)
        end
    end
end

function PowerUpManager:SpawnRandom(position)
    if #self.Pool == 0 then
        return
    end

    local prefab = self.Pool[Math.RandomInt(1, #self.Pool)]
    return Scene.InstantiatePrefab(prefab, position)
end
```

### Script Inheritance (`Base`)

A script can share methods and default properties with other scripts by declaring `Base` as the
**name** of another script asset (not a path — the same name shown in the Asset Manager):

```lua
-- PurchasableItem.lua — never attached to an entity directly; other scripts inherit from it.
local PurchasableItem = {}

PurchasableItem.ItemCost = 0

function PurchasableItem:CanAfford()
    return PointManager ~= nil and PointManager.Points >= self.ItemCost
end

return PurchasableItem
```

```lua
-- PickupWeapon.lua
local PickupWeapon = {}
PickupWeapon.Base = "PurchasableItem"

PickupWeapon.ItemCost = 250 -- overrides the inherited default of 0

function PickupWeapon:OnPickup(entity, otherEntity)
    if not self:CanAfford() then
        return
    end
    -- ...
end

return PickupWeapon
```

Methods and properties not defined on the child fall back to `Base`, then to *its* `Base`, and so
on — an ordinary prototype chain, so a method the child *does* define always wins over one
inherited from a parent. Properties work the same way for the Inspector: a field declared only on
the base (like `ItemCost` above) is still exposed on every script that inherits from it, using the
base's value as the default; a child that declares its own value for that field overrides the
default instead of duplicating the base's logic.

A few things worth knowing:

- `Base` is resolved by looking up a script asset with that exact name, so the base script must
  exist somewhere in the project (it does not need to be attached to any entity).
- A cycle (`A.Base = "B"`, `B.Base = "A"`) or a `Base` naming a script that doesn't exist is logged
  as an error and the chain simply stops there — the script itself still loads and runs normally,
  just without whatever it would have inherited.
- `Base` itself is a reserved field name, like the lifecycle hooks above — it never shows up as an
  editable Inspector property.
- [`entity:GetScriptInstance(name)`](#entity) matches `name` against the whole `Base` chain, not
  just the script's own concrete name — so `hitEntity:GetScriptInstance("PurchasableItem")` finds
  the script on an entity whose attached script is actually `PickupWeapon`, letting other scripts
  treat every kind of purchasable pickup generically without knowing the concrete subclass.

---

## Core Types

### `UUID`

A 64-bit unique identifier.

| Member                        | Description                    |
| ----------------------------- | ------------------------------ |
| `UUID()` / `UUID(uint64)` | Constructors.                  |
| `a == b`                    | Equality check.                |
| `tostring(uuid)`            | Decimal string representation. |

### `TimeStep`

A wrapper around an elapsed-time value used by the engine.

| Member                                  | Description                           |
| --------------------------------------- | ------------------------------------- |
| `TimeStep()` / `TimeStep(seconds)`  | Constructors.                         |
| `:Seconds()`                          | Returns the value in seconds.         |
| `:Milliseconds()`                     | Returns the value in milliseconds.    |
| `a + b`, `a == b`, `tostring(ts)` | Arithmetic / comparison / formatting. |

### `Log`

Logging utilities that route through Ember's core logger.

```lua
Log.Info("Hello from Lua!")
Log.Warn("Something looks off...")
Log.Error("Something went wrong!")
```

### `Time` / `Timer`

Runtime time helpers.

```lua
local now = Time.Now() -- engine timer value in seconds

Timer.SetTimeout(function()
    Log.Info("Called after half a second")
end, 0.5)
```

### `Window` / `Renderer`

Query the game window and the runtime viewport.

```lua
local w = Window.GetWidth()           -- number (pixels)
local h = Window.GetHeight()          -- number (pixels)

local viewport = Renderer.GetViewportSize()  -- Vector2f { x = width, y = height }
```

---

## Math

All math primitives are bound directly from `Ember::Math`.

### Vectors & Matrices

| Type         | Constructors                                     | Fields                     |
| ------------ | ------------------------------------------------ | -------------------------- |
| `Vector2f` | `Vector2f.new()`, `Vector2f.new(x, y)`       | `x`, `y`               |
| `Vector3f` | `Vector3f.new()`, `Vector3f.new(x, y, z)`    | `x`, `y`, `z`        |
| `Vector4f` | `Vector4f.new()`, `Vector4f.new(x, y, z, w)` | `x`, `y`, `z`, `w` |
| `Matrix3f` | `Matrix3f.new()`, `Matrix3f.new(diagonal)`   | —                         |
| `Matrix4f` | `Matrix4f.new()`, `Matrix4f.new(diagonal)`   | —                         |

All vectors support `+`, `-`, `*` (scalar), `/` (scalar), and unary `-`.
`Vector3f` also supports component-wise `a * b`.
Matrices support `+`, `-`, and `*` (against another matrix, a vector, or a scalar).

```lua
local a = Vector3f.new(1, 0, 0)
local b = Vector3f.new(0, 1, 0)
local c = a + b                 -- (1, 1, 0)
local d = c * 2.0               -- (2, 2, 0)
```

### `Quaternion`

| Member                         | Description                                         |
| ------------------------------ | --------------------------------------------------- |
| `Quaternion.new()`           | Identity quaternion.                                |
| `Quaternion.new(x, y, z, w)` | Component-wise constructor.                         |
| `Quaternion.new(eulerVec3)`  | Construct from Euler angles.                        |
| `x`, `y`, `z`, `w`     | Components.                                         |
| `q1 * q2`                    | Quaternion multiplication.                          |
| `q * v3`                     | Rotates a`Vector3f` by the quaternion.            |
| `:Inverse()`                 | Returns the inverse rotation matrix (`Matrix4f`). |
| `:Normalize()`               | Returns a normalized copy.                          |

### `Math` table

```lua
Math.PI                 -- numeric constant

Math.Max(a, b)          -- floats or Vector3f
Math.Min(a, b)
Math.Clamp(v, lo, hi)   -- floats or Vector3f
Math.Lerp(a, b, t)      -- floats or Vector3f
Math.Slerp(qA, qB, t)

Math.Random()
Math.RandomFloat(min, max)
Math.RandomInt(min, max)

Math.Sin(rad)
Math.Cos(rad)
Math.Tan(rad)
Math.Asin(v)
Math.Acos(v)
Math.Atan2(y, x)

Math.Radians(deg)
Math.Degrees(rad)
Math.Length(v3)
Math.Magnitude(v3)
Math.Magnitude2(v3)
Math.Distance(a, b)
Math.Distance2(a, b)
Math.Cross(a, b)
Math.Dot(a, b)
Math.Normalize(v)       -- Vector3f or Quaternion
Math.ProjectOnPlane(vec, planeNormal)

Math.Inverse(matrix4)
Math.LookAt(eye, target, up)
Math.LookAt(start, end)
Math.GetRotationMatrix(quat)

Math.Translate(v3)                  -- returns Matrix4f
Math.Translate(matrix, v3)
Math.Rotate(angleRad, axis)         -- returns Matrix4f
Math.Rotate(matrix, angleRad, axis)
Math.Scale(v3)                      -- returns Matrix4f
Math.Scale(matrix, v3)

Math.ToQuaternion(matrix4)
Math.ToQuaternion(eulerVec3)
Math.ToEulerAngles(quat)
Math.ToMatrix4f(quat)

local ok, t, r, s = Math.DecomposeTransform(matrix4)
```

---

## Entity & Scene

### `Entity`

The handle to a world object. Almost all gameplay code goes through `Entity`.

| Method                           | Description                                                                              |
| -------------------------------- | ---------------------------------------------------------------------------------------- |
| `:GetName()`                   | Returns the entity's name (string).                                                      |
| `:GetUUID()`                   | Returns the entity's`UUID`.                                                            |
| `:SetActive(bool)`             | Enables or disables the entity and all descendants (toggles`DisabledComponent`).       |
| `:SetActive(bool, recursive)`  | Enables/disables the entity; pass`false` for `recursive` to only affect this entity. |
| `:IsActive()`                  | Returns`true` when the entity does not have `DisabledComponent`.                     |
| `:AttachComponent(typeName)`   | Adds a component by string name; returns the new component (or existing one).            |
| `:DetachComponent(typeName)`   | Removes a component by string name.                                                      |
| `:GetComponent(typeName)`      | Returns the component, or logs an error and returns`nil` if it isn't present.          |
| `:ContainsComponent(typeName)` | Returns`true` if the component is attached.                                            |
| `:GetParent()`                 | Returns the parent`Entity` (may be invalid).                                           |
| `:IsRootParent()`              | `true` if this entity has no parent.                                                   |
| `:GetRootParent()`             | Walks up the hierarchy to the topmost ancestor.                                          |
| `:GetChildren()`               | Returns a Lua-indexable list of child entities.                                          |
| `:GetChild(name)`              | Returns the first child matching`name`.                                                |
| `:AddChild(entity)`            | Re-parents`entity` under this one.                                                     |
| `:GetScriptInstance()`         | Returns the script's table on this entity, or`nil` if no `ScriptComponent`.          |
| `:GetScriptInstance(name)`     | Same, but `nil` unless `name` matches the script's own name or one of its `Base` ancestors. |

#### Component type names

Pass these strings to `GetComponent`, `AttachComponent`, `DetachComponent`, `ContainsComponent`:

```
IDComponent              TagComponent              RelationshipComponent
TransformComponent       RigidBodyComponent        SpriteComponent
StaticMeshComponent      SkinnedMeshComponent      MaterialComponent
CameraComponent          DirectionalLightComponent SpotLightComponent
PointLightComponent      OutlineComponent
AnimatorComponent        BoneSocketComponent       CharacterControllerComponent
TextComponent            AudioSourceComponent
WaypointComponent        AIPathComponent           AIAgentComponent
LocalAvoidanceComponent
BoxColliderComponent     SphereColliderComponent   CapsuleColliderComponent
ConvexMeshColliderComponent  ConcaveMeshColliderComponent
LifetimeComponent        ParticleEmitterComponent
CanvasComponent          RectTransformComponent
UISelectableComponent    UIButtonComponent         UIToggleComponent
```

> `ScriptComponent` and `DisabledComponent` cannot be added/removed/queried from Lua directly.
> Use `entity:GetScriptInstance()` and `entity:SetActive(bool)` instead.

### `Scene`

A global table of helpers for the **currently loaded scene**.

```lua
Scene.AddEntity(name)                     -- returns the new Entity
Scene.RemoveEntity(entity)                -- accepts an Entity or an EntityID
Scene.DuplicateEntity(name)               -- duplicates the entity with the given name
Scene.GetEntityByName(name)               -- lookup by name

Scene.InstantiatePrefab(prefabAssetName, position) -- spawns a prefab at the given Vector3f
Scene.InstantiatePrefab(prefabAssetName, parentEntity)
Scene.InstantiatePrefab(prefabAssetName, parentEntity, position)

Scene.SetActiveCamera(entityName)         -- sets which entity drives the runtime camera

-- Object pools (efficient prefab spawning)
Scene.CreatePool(poolID, prefabAssetName, initialSize)
Scene.CreatePool(poolID, prefabAssetName, initialSize, loopEntities)
Scene.RetrieveFromPool(poolID)            -- returns an inactive Entity from the pool
Scene.RetrieveFromPool(poolID, position)  -- returns one positioned at `position`
```

### `SceneManager`

Defers a scene transition until the current frame finishes.

```lua
SceneManager.LoadScene("LevelTwo")  -- by asset name
SceneManager.LoadNextScene()        -- next entry in the project's scene list
SceneManager.LoadDefaultScene()     -- first entry in the project's scene list
SceneManager.IsLastScene()          -- bool
```

---

## Components

Components are obtained via `entity:GetComponent("TypeName")`. Fields are read/write unless noted.

### `TransformComponent`

| Field / Method    | Type                     | Notes                                         |
| ----------------- | ------------------------ | --------------------------------------------- |
| `Position`      | `Vector3f`             | Local position.                               |
| `Rotation`      | `Vector3f`             | Local Euler angles (radians).                 |
| `Scale`         | `Vector3f`             | Local scale.                                  |
| `WorldPosition` | `Vector3f` (read-only) | Position derived from the world matrix.       |
| `WorldRotation` | `Vector3f` (read-only) | Euler rotation derived from the world matrix. |
| `:GetForward()` | `Vector3f`             | Forward direction in world space.             |
| `:GetRight()`   | `Vector3f`             | Right direction in world space.               |
| `:GetUp()`      | `Vector3f`             | Up direction in world space.                  |

### Rendering Components

#### `SpriteRendererComponent` (`SpriteComponent`)

| Field             | Type         |
| ----------------- | ------------ |
| `Color`         | `Vector4f` |
| `TextureHandle` | `UUID`     |
| `IsBillboard`   | `bool`     |
| `LockYAxis`     | `bool`     |

#### `StaticMeshComponent`

| Field          | Type     |
| -------------- | -------- |
| `MeshHandle` | `UUID` |

#### `SkinnedMeshComponent`

| Field                    | Type     |
| ------------------------ | -------- |
| `MeshHandle`           | `UUID` |
| `AnimatorEntityHandle` | `UUID` |

#### `MaterialComponent`

| Field / Method       | Type / Description                                           |
| -------------------- | ------------------------------------------------------------ |
| `MaterialHandle`   | `UUID`                                                     |
| `:GetInstanced()`  | Whether this entity has its own material instance.           |
| `:CloneMaterial()` | Creates a per-entity material instance for runtime tweaking. |

#### `OutlineComponent`

| Field         | Type         |
| ------------- | ------------ |
| `Color`     | `Vector4f` |
| `Thickness` | `float`    |

#### `TextComponent`

| Field                    | Type              |
| ------------------------ | ----------------- |
| `Text`                 | `string`        |
| `Color`                | `Vector4f`      |
| `FontSize`             | `float`         |
| `HorizontalAlignment`  | `TextAlignment` |
| `VerticalAlignment`    | `TextAlignment` |

`FontSize` is the pixel height of screen-space text. It is independent of the element's
`RectTransform` size, so resizing a button does not resize its label.

`TextAlignment` is `Start` / `Center` / `End` (left/bottom, centre, right/top). Alignment
positions the measured text block inside the element's rect.

#### `ParticleEmitterComponent`

| Field                         | Type         |
| ----------------------------- | ------------ |
| `EmissionRate`              | `float`    |
| `Velocity`                  | `Vector3f` |
| `VelocityVariation`         | `Vector3f` |
| `ColorBegin` / `ColorEnd` | `Vector4f` |
| `ScaleBegin` / `ScaleEnd` | `float`    |
| `ScaleVariation`            | `float`    |
| `TextureHandle`             | `UUID`     |
| `Lifetime`                  | `float`    |
| `LifetimeVariation`         | `float`    |
| `GravityMultiplier`         | `float`    |
| `IsActive`                  | `bool`     |

### Lighting Components

#### `DirectionalLightComponent`

| Field         | Type         |
| ------------- | ------------ |
| `IsActive`  | `bool`     |
| `Color`     | `Vector4f` |
| `Intensity` | `float`    |

#### `PointLightComponent`

| Field         | Type         |
| ------------- | ------------ |
| `IsActive`  | `bool`     |
| `Color`     | `Vector4f` |
| `Intensity` | `float`    |
| `Radius`    | `float`    |

#### `SpotLightComponent`

| Field                | Type         |
| -------------------- | ------------ |
| `IsActive`         | `bool`     |
| `Color`            | `Vector4f` |
| `Intensity`        | `float`    |
| `CutOffAngle`      | `float`    |
| `OuterCutOffAngle` | `float`    |

### `CameraComponent`

| Field                                      | Type                                                           |
| ------------------------------------------ | -------------------------------------------------------------- |
| `IsActive`                               | `bool`                                                       |
| `ProjectionType`                         | `Camera.ProjectionType` (`Perspective` / `Orthographic`) |
| `FieldOfView`                            | `float` (perspective)                                        |
| `PerspectiveNear` / `PerspectiveFar`   | `float`                                                      |
| `OrthographicSize`                       | `float`                                                      |
| `OrthographicNear` / `OrthographicFar` | `float`                                                      |

### Physics Components

#### `RigidBodyComponent`

| Field / Method                                | Description                                                                                          |
| --------------------------------------------- | ---------------------------------------------------------------------------------------------------- |
| `Mass`                                      | `float`                                                                                            |
| `GravityEnabled`                            | `bool`                                                                                             |
| `:ApplyForce(force)`                        | Apply a continuous force (`Vector3f`).                                                             |
| `:ApplyImpulse(impulse)`                    | Apply an instantaneous impulse (`Vector3f`).                                                       |
| `:ApplyImpulseAtPoint(impulse, worldPoint)` | Apply an instantaneous impulse at a world-space point so it generates torque. Both args`Vector3f`. |
| `:CurrentVelocity()`                        | Returns the current rigid body velocity (`Vector3f`).                                              |

#### `ColliderOffset`

| Field        | Type         |
| ------------ | ------------ |
| `Position` | `Vector3f` |
| `Rotation` | `Vector3f` |

#### `BoxColliderComponent`

| Field             | Type               |
| ----------------- | ------------------ |
| `Size`          | `Vector3f`       |
| `Offset`        | `ColliderOffset` |
| `Category`      | `Filter`         |
| `CollisionMask` | `Filter`         |

#### `SphereColliderComponent`

| Field                           | Type               |
| ------------------------------- | ------------------ |
| `Radius`                      | `float`          |
| `Offset`                      | `ColliderOffset` |
| `Category`, `CollisionMask` | `Filter`         |

#### `CapsuleColliderComponent`

| Field                           | Type               |
| ------------------------------- | ------------------ |
| `Radius`                      | `float`          |
| `Height`                      | `float`          |
| `Offset`                      | `ColliderOffset` |
| `Category`, `CollisionMask` | `Filter`         |

#### `ConvexMeshColliderComponent` / `ConcaveMeshColliderComponent`

| Field                           | Type               |
| ------------------------------- | ------------------ |
| `MeshHandle`                  | `UUID`           |
| `Offset`                      | `ColliderOffset` |
| `Category`, `CollisionMask` | `Filter`         |

#### `CharacterControllerComponent`

| Field / Method          | Description                                   |
| ----------------------- | --------------------------------------------- |
| `WalkSpeed`           | `float`                                     |
| `JumpForce`           | `float`                                     |
| `Velocity`            | `Vector3f`                                  |
| `RequestedMovement`   | `Vector3f`                                  |
| `GravityMultiplier`   | `float`                                     |
| `MaxSlopeAngle`       | `float`                                     |
| `MaxStepHeight`       | `float`                                     |
| `IsGrounded`          | `bool` (read-only state)                    |
| `GroundEntity`        | `Entity` the controller is standing on.     |
| `MovementVelocity`    | Current movement velocity (`Vector3f`).     |
| `:Move(displacement)` | Move by`Vector3f` (call from `OnUpdate`). |
| `:Jump()`             | Triggers a jump using`JumpForce`.           |

### Audio Components

#### `AudioSourceComponent`

| Field / Method                                      | Description                                      |
| --------------------------------------------------- | ------------------------------------------------ |
| `AudioClipHandle`                                 | `UUID` of the audio asset.                     |
| `Volume`, `Pitch`, `Looping`, `Spatialized` | Property accessors.                              |
| `:Play()`                                         | Plays the assigned clip with current properties. |
| `:Stop()`                                         | Stops playback.                                  |
| `:Restart()`                                      | Restart from the beginning.                      |

#### `AudioListenerComponent`

Marks an entity (usually the camera) as a 3D-audio listener for spatialized sounds.

| Field             | Type                                               |
| ----------------- | -------------------------------------------------- |
| `IsActive`      | `bool`                                           |
| `ListenerIndex` | `int` (miniaudio listener index, normally `0`) |

### AI Components

#### `AIPathComponent`

| Field / Method                 | Description                                                     |
| ------------------------------ | --------------------------------------------------------------- |
| `Waypoints`                  | List of`Vector3f` waypoints (Lua array).                      |
| `CurrentWaypointIndex`       | Current index along the path.                                   |
| `Speed`                      | `float`                                                       |
| `ArrivalTolerance`           | `float`                                                       |
| `:GetNextWaypointPosition()` | Returns the`Vector3f` of `Waypoints[CurrentWaypointIndex]`. |

#### `AIAgentComponent`

| Field                   | Description                                                                 |
| ----------------------- | --------------------------------------------------------------------------- |
| `Mode`                | `PathMode.Manual` or `PathMode.Dynamic`. Setting flags the agent dirty. |
| `Waypoints`           | Manual waypoints (when in`Manual` mode).                                  |
| `Loop`                | `bool`                                                                    |
| `TargetEntity`        | `Entity` to chase (when in `Dynamic` mode).                             |
| `GridEntity`          | `Entity` representing the navigation grid.                                |
| `RecalculateInterval` | `float`                                                                   |

`PathMode` is a global enum with `Manual` and `Dynamic` values.

#### `LocalAvoidanceComponent`

| Field                 | Type         |
| --------------------- | ------------ |
| `AvoidanceRadius`   | `float`    |
| `AvoidanceStrength` | `float`    |
| `AvoidanceVector`   | `Vector3f` |
| `AvoidanceMask`     | `Filter`   |

### Misc Components

#### `AnimatorComponent`

| Field / Method                                    | Description                                               |
| ------------------------------------------------- | --------------------------------------------------------- |
| `CurrentAnimationHandle`                        | `UUID` of the active animation.                         |
| `CurrentTime`                                   | `float` (read/write).                                   |
| `PlaybackSpeed`                                 | `float`                                                 |
| `IsPlaying`                                     | `bool`                                                  |
| `Loop`                                          | `bool`                                                  |
| `:Crossfade(name, duration)`                    | Smoothly blends to another animation by name.             |
| `:Crossfade(name)`                              | Switches to another animation by name without blend time. |
| `:Crossfade(targetAnimUUID, duration)`          | Smoothly blends to another animation by asset UUID.       |
| `:Play(name)`                                   | Plays an animation once by name.                          |
| `:Play(name, playbackSpeed)`                    | Plays once with a custom speed.                           |
| `:Play(name, playbackSpeed, blendDuration)`     | Plays once with speed and blend duration.                 |
| `:PlayLoop(name)`                               | Plays an animation in a loop by name.                     |
| `:PlayLoop(name, playbackSpeed)`                | Loops with a custom speed.                                |
| `:PlayLoop(name, playbackSpeed, blendDuration)` | Loops with speed and blend duration.                      |

#### `BoneSocketComponent`

Attaches one entity to a named bone on another animated entity.

| Field                  | Type                               |
| ---------------------- | ---------------------------------- |
| `TargetEntityHandle` | `UUID`                           |
| `BoneName`           | `string`                         |
| `Position`           | `Vector3f` local socket offset.  |
| `Rotation`           | `Vector3f` local Euler rotation. |
| `Scale`              | `Vector3f` local scale.          |

#### `LifetimeComponent`

| Field        | Type                                               |
| ------------ | -------------------------------------------------- |
| `Lifetime` | `float` (seconds remaining before auto-destroy). |

### UI Components

A UI element is a `RectTransformComponent` under a `CanvasComponent`. Interaction is composed:
`UISelectableComponent` supplies the shared state machine (hover / press / focus / disabled and the
visual transition), and a role component on the same entity says what activating it *does*.

#### `CanvasComponent`

| Field                   | Type         |
| ----------------------- | ------------ |
| `ReferenceResolution` | `Vector2f` |
| `MatchWidthOrHeight`  | `float`    |
| `SortOrder`           | `int`      |

Authored sizes are in reference-resolution pixels and scale with the viewport.
Canvases with a higher `SortOrder` draw on top and win clicks.

#### `RectTransformComponent`

| Field                | Type         |
| -------------------- | ------------ |
| `AnchorMin`        | `Vector2f` |
| `AnchorMax`        | `Vector2f` |
| `Pivot`            | `Vector2f` |
| `SizeDelta`        | `Vector2f` |
| `AnchoredPosition` | `Vector2f` |
| `Rotation`         | `float` (radians) |
| `RaycastTarget`    | `bool`     |

`RaycastTarget` makes a plain rect hit-testable without a selectable - useful for invisible click
zones and modal blockers. It defaults to `false`, and selectables are always hit-testable.

#### `UISelectableComponent`

| Field                                                | Type         |
| ---------------------------------------------------- | ------------ |
| `Interactable`                                     | `bool`     |
| `NormalColor` / `HighlightedColor` / `PressedColor` | `Vector4f` |
| `SelectedColor` / `DisabledColor`                  | `Vector4f` |
| `FadeDuration`                                     | `float`    |
| `IsHovered`                                        | `bool` (read-only) |
| `IsPressed`                                        | `bool` (read-only) |
| `:Select()`                                        | Makes this the focused selectable. |

State colours are **multiplied** by the target graphic's authored colour, so recolouring a button
from script still works. A non-interactable selectable still blocks clicks rather than letting
them fall through.

`:Select()` is what a menu calls when it opens on a gamepad, so the first press of A activates
something instead of just adopting a selectable. It gives the selectable focus - the same focus
keyboard/gamepad navigation moves and `UI.ClearFocus()` drops - which is what drives the
`SelectedColor` tint. On a non-interactable selectable it does nothing.

#### `UIButtonComponent`

| Field / Method            | Description                                            |
| ------------------------- | ------------------------------------------------------ |
| `WasClickedThisFrame`   | `bool`, true for the frame a click completes.        |
| `:OnClick(fn)`          | Registers `fn(entity)`; call repeatedly to add more. |
| `:ClearOnClick()`       | Drops every handler registered on this entity.         |

#### `UIToggleComponent`

| Field / Method                 | Description                                              |
| ------------------------------ | -------------------------------------------------------- |
| `IsOn`                       | `bool` (read/write; writing moves the checkmark).      |
| `AllowSwitchOff`             | `bool`, lets a grouped toggle be switched off.         |
| `WasChangedThisFrame`        | `bool`                                                 |
| `:OnValueChanged(fn)`        | Registers `fn(entity, isOn)`.                          |
| `:ClearOnValueChanged()`     | Drops every handler registered on this entity.          |

Toggles sharing a `Group` entity are mutually exclusive (radio behaviour).

Handlers registered this way live for one play session and are dropped when play stops, so
register them from `OnCreate`. For static UI, the `OnClick` / `OnValueChanged` lifecycle hooks on
the entity's own script need no registration at all.

```lua
local Menu = {}

function Menu:OnCreate(entity)
    -- Registered handler: the natural fit for UI built at runtime, e.g. a row per store item.
    local button = entity:GetChild("BuyButton"):GetComponent("UIButtonComponent")
    button:OnClick(function(clicked)
        Log.Info("bought!")
    end)
end

-- Lifecycle hook: fires on this entity's own script with no registration.
function Menu:OnClick(entity)
    SceneManager.LoadScene("LevelOne")
end

return Menu
```

---

## UI

```lua
UI.IsPointerOverUI()                      -- bool
UI.ClearFocus()
```

`IsPointerOverUI` is true when the pointer is over any element that accepts raycasts. Check it
before acting on a click so shooting, selecting or camera-dragging ignores clicks that landed on
the HUD.

Submit (Enter / Space / gamepad A) is handled for you: when it activates a focused widget, that
press is consumed, and any input action bound to the same control reads as inactive until the
control is released. So closing a menu with A does not also fire the jump bound to A on the same
frame - no `IsPointerOverUI`-style guard needed for it. Navigation and cancel are *not* consumed,
so a game binding its own action to Escape or B still sees those presses.

A focused selectable that gets hidden or made non-interactable loses the focus on the next frame,
so hiding a menu is enough to stop submit reaching it. `UI.ClearFocus()` is still worth calling
when you want the focus gone immediately.

---

## Input

```lua
Input.IsKeyDown(KeyCode.W)                -- bool, currently held
Input.IsKeyReleased(KeyCode.Space)        -- bool, released this frame
Input.IsKeyRepeating(KeyCode.Backspace)   -- bool, OS key-repeat is firing

Input.IsMouseButtonDown(MouseButton.Left)
Input.IsMouseButtonReleased(MouseButton.Left)

Input.IsMouseControlDown(MouseControl.Button4)
Input.IsMouseControlPressed(MouseControl.WheelUp)     -- bool, this frame only
Input.IsMouseControlReleased(MouseControl.Button4)

Input.IsModifierDown(KeyModifier.Shift)   -- bool, modifier currently held
Input.GetActiveModifiers()                -- int, the whole modifier bitmask

Input.IsActionDown("Jump")                -- bool, any of the action's triggers is active
Input.IsActionPressed("Jump")             -- bool, went down this frame
Input.IsActionReleased("Jump")            -- bool, came up this frame
Input.ConsumeAction("Interact")           -- swallow the press you just acted on

Input.GetActionStrength("MoveRight")      -- number, 0..1 (a key is 1, a stick is how far it moved)
Input.GetAxis("MoveLeft", "MoveRight")    -- number, -1..1 from a pair of actions
Input.GetAxis2D("MoveLeft", "MoveRight", "MoveBackward", "MoveForward")  -- Vector2f

Input.GetMousePosition()                  -- Vector2f
Input.GetMouseScrollOffset()              -- Vector2f
Input.GetMouseDelta()                     -- Vector2f, this frame's travel, inversion applied
Input.GetRawMouseDelta()                  -- Vector2f, the same travel in true pixels

Input.GetStickSettings(GamepadStick.Right)     -- StickSettings, writable
Input.GetTriggerSettings(GamepadTrigger.Left)  -- TriggerSettings, writable
Input.GetMouseSettings()                       -- MouseSettings, writable

Input.IsAnyGamepadActive()                -- bool, is any pad connected
Input.IsGamepadActive(0)                  -- bool, is this pad connected
Input.IsGamepadButtonDown(0, GamepadButton.A)      -- bool, currently held
Input.IsGamepadButtonPressed(0, GamepadButton.A)   -- bool, went down this frame
Input.IsGamepadButtonReleased(0, GamepadButton.A)  -- bool, not held
Input.GetGamepadAxis(0, GamepadAxis.LeftX)         -- number, -1..1 (triggers are 0..1)

Input.GetLastUsedInputDevice()            -- InputDevice, what the player touched last
Input.SetLastUsedInputDevice(InputDevice.Gamepad)

Input.SetCursorMode(mode)
Input.GetCursorMode()

Input.GetViewportMousePosition()          -- Vector2f, UI space
```

`Input.IsKeyPressed` and `Input.IsMouseButtonPressed` are older spellings that mean **held**, not
"pressed this frame" — `IsKeyDown` / `IsMouseButtonDown` are the clearer names for the same thing.
For a genuine one-frame edge on the mouse use `IsMouseControlPressed`, which is also the only way
to read the scroll wheel as a control: a wheel notch is a one-frame pulse and is never *down*.

`IsModifierDown` tests the argument against the active bitmask, so a multi-bit argument asks
*any of these*: `Input.IsModifierDown(KeyModifier.Shift | KeyModifier.Control)` is true when either
is held. For a chord that has to match exactly, compare the mask itself —
`Input.GetActiveModifiers() == (KeyModifier.Shift | KeyModifier.Control)`.
`Input.IsModifierActive` is the same function under the C++ spelling.

The `IsAction*` queries read the input actions defined in **Project Settings → Input**, so a game
can rebind controls without touching script. An unknown action name logs an error and returns
false rather than failing the call.

Reading an action never spends it. `IsActionPressed` is one flag per action for the whole frame, so
every script that asks during that frame gets the same answer — two actions sharing one physical
button both report the press, and a script that opens a menu mid-frame will have that menu's own
close check read the very same press further down the frame. `Input.ConsumeAction` is the way out:
call it right after acting on a press, and whichever control is actuating that action goes quiet for
every action bound to it until the player physically releases it. It does nothing if the action is
not actuating, so it is safe to call unconditionally.

```lua
if Input.IsActionPressed("Interact") then
    interactable:OnInteract(interactableEntity, entity)

    -- Interact and NavBack share a button, so this stops the menu we just opened seeing the press.
    Input.ConsumeAction("Interact")
end
```

`GetActionStrength` is the analog version of `IsActionDown`: a key or button reports 1, a gamepad
axis reports how far it actually moved, and the strongest trigger on the action wins. `GetAxis`
subtracts one action's strength from another's, and `GetAxis2D` pairs two of those into a movement
vector — so a stick and WASD drive the same code as long as each direction is its own action. In the
trigger picker a stick axis can be bound whole or as a single half (`Left Stick Left` is the
negative half of `LeftX`), and a half only reports while the stick is pushed that way.

Prefer the `IsAction*` / `GetAxis*` calls over the raw `Input.IsGamepad*` reads: they are
rebindable, and they already handle the keyboard. The raw reads are for the cases actions cannot
express - a specific physical button on a specific pad, or a stick you want unfiltered by an action
mapping. The pad index is **0-based**, so player one is `0`, and up to four pads are polled.

`GetGamepadAxis` returns the **conditioned** value: a stick reads -1..1 with a radial deadzone
applied to the pair (so pushing one axis fully never leaves the other exactly zero), and a trigger
reads 0..1 rather than the -1..1 the OS reports. `IsGamepadButtonReleased` means "not held right
now", not "came up this frame".

### Conditioning

The engine corrects what the hardware gets wrong, because every reader of a control wants the same
correction. Each stick and trigger has its own **deadzone** (deflection that reads as centred),
**saturation** (deflection that already counts as fully pushed), **exponent** (the response curve)
and **actuation** (physical travel before a digital read calls it pressed), plus per-axis
inversion. Defaults live in **Project Settings → Input → Devices** and are saved to the `.ebproj`;
`GetStickSettings` and friends let a settings menu change them at runtime.

```lua
local look = Input.GetStickSettings(GamepadStick.Right)
look.Exponent = 2.0   -- finer control near centre, still full rate at full deflection
look.InvertY = true
```

The exponent shapes the stick's **magnitude**, not each axis on its own, so a diagonal keeps its
angle and a circular sweep of the stick stays circular. `1.0` is linear. Above `1.0` gives finer
control near centre, which is what first-person look wants; below `1.0` makes it twitchier.

What the signal *means* stays in the game: degrees-per-second turn rates, pitch clamping, ADS
multipliers and aim assist all belong in your look script. Mouse sensitivity is deliberately not an
engine setting - the mouse delta has several readers (look, weapon sway, UI dragging) and only each
one knows its own scale.

> Set inversion on the stick that actually drives the camera. The **left** stick also drives menu
> navigation, so inverting it flips the menus too.

`GetMouseDelta` is the whole frame's travel with the player's inversion applied - it is an angle
already, so do **not** scale it by delta time. A stick is a position held over time, so it *is* a
rate and does need delta time. `GetRawMouseDelta` skips inversion for callers that want true pixels.

`GetLastUsedInputDevice` reports whichever device produced the most recent input - use it to switch
button prompts between key glyphs and pad glyphs without polling every control yourself. It updates
on key, mouse and pad activity; `SetLastUsedInputDevice` lets a game force it, e.g. to pin the
prompts to one device in a settings menu.

> A digital read of an analog control needs the control to travel past an **actuation point**
> before it counts as pressed: half a stick's throw by default, or a light pull on a trigger.
> Without it a stick pushed "straight" forward - which always leaves a few percent on the other
> axis - would hold the perpendicular direction down too and movement could only ever come out
> diagonal. The actuation point is physical travel, so it runs through the same response curve the
> axis does and a curve never moves it. `GetActionStrength` is unaffected and still reports the
> full analog value.

> A trigger's required modifiers are a **subset** test: all of them must be held, and anything else
> held alongside is ignored. So `Key/W` keeps firing while Shift is held (sprint does not cancel
> movement), but a `Key/Ctrl+S` action does not suppress a plain `Key/S` one — pressing Ctrl+S fires
> both. Check the modifiers yourself in script if a chord has to win.

`GetMousePosition` returns raw window coordinates (top-left origin, +Y down).
`GetViewportMousePosition` returns the pointer in **UI space** - viewport-local, bottom-left
origin, +Y up - which is the space `RectTransform` rects are laid out in. Use it for any
screen-space hit testing: inside the editor's docked viewport during Play, the two differ by the
viewport panel's position.

> A locked cursor (`Input.SetCursorMode(CursorMode.Locked)`) reports unbounded virtual
> coordinates, so UI is not clickable while locked. Both hosts lock the cursor on start for
> first-person play; call `Input.SetCursorMode(CursorMode.Normal)` when opening a menu.

### Enums

- **`KeyCode`** — `Unknown`, `Space`, `Apostrophe`, `Comma`, `Minus`, `Period`, `Slash`,
  `D0`–`D9`, `Semicolon`, `Equal`, `A`–`Z`, `LeftBracket`, `Backslash`, `RightBracket`,
  `GraveAccent`, `Escape`, `Enter`, `Tab`, `Backspace`, `Insert`, `Delete`, arrow keys
  (`Right`, `Left`, `Down`, `Up`), `PageUp`, `PageDown`, `Home`, `End`, `CapsLock`,
  `ScrollLock`, `NumLock`, `PrintScreen`, `Pause`, `F1`–`F12`, `NumPad0`–`NumPad9`,
  `NumPadDecimal`, `NumPadDivide`, `NumPadMultiply`, `NumPadSubtract`, `NumPadAdd`,
  `NumPadEnter`, `NumPadEqual`, `LeftShift`, `LeftControl`, `LeftAlt`, `LeftSuper`,
  `RightShift`, `RightControl`, `RightAlt`, `RightSuper`, `Menu`, `Last`.
- **`KeyAction`** — `Release`, `Press`, `Repeat`.
- **`KeyModifier`** — `None`, `Shift`, `Control`, `Alt`, `Super`. A bitmask, so values combine
  with `|`. `None` is zero and therefore never reads as down.
- **`MouseButton`** — `Left`, `Right`, `Middle`.
- **`MouseControl`** — `Left`, `Right`, `Middle`, `Button4`–`Button15`, `WheelUp`, `WheelDown`.
  A superset of `MouseButton` (the first three values match), covering the side buttons and the
  wheel that `MouseButton` cannot name.
- **`CursorMode`** — `Normal` (`0`), `Hidden` (`1`), `Locked` (`2`). `Input.SetCursorMode` also
  accepts the raw integer.
- **`GamepadButton`** — `A`, `B`, `X`, `Y`, `LeftBumper`, `RightBumper`, `Back`, `Start`, `Guide`,
  `LeftThumb`, `RightThumb`, `DPadUp`, `DPadRight`, `DPadDown`, `DPadLeft`, `Last`. Named after the
  Xbox layout, which is the layout GLFW maps every pad onto — on a PlayStation pad `A` is Cross and
  `B` is Circle.
- **`GamepadAxis`** — `LeftX`, `LeftY`, `RightX`, `RightY`, `LeftTrigger`, `RightTrigger`, `Last`.
- **`GamepadStick`** — `Left`, `Right`. Which stick `GetStickSettings` conditions.
- **`GamepadTrigger`** — `Left`, `Right`. Which trigger `GetTriggerSettings` conditions.
- **`InputDevice`** — `None`, `Keyboard`, `Mouse`, `Gamepad`. What `GetLastUsedInputDevice` returns.

---

## Physics

### `Physics`

```lua
local hit = Physics.CastRay(startV3, endV3)                     -- RaycastHit (filter = All)
local hit = Physics.CastRay(startV3, endV3, CollisionFilter.Enemy) -- filtered cast

Physics.CheckOverlapBox(position, rotation, scale, entity)
Physics.CheckOverlapBox(position, rotation, scale, entity, filter)
Physics.CheckOverlapBoxWithData(position, rotation, scale, entity)
Physics.CheckOverlapBoxWithData(position, rotation, scale, entity, filter)

Physics.CheckOverlapSphere(entity)
Physics.CheckOverlapSphere(position, radius, entity)
Physics.CheckOverlapSphere(position, radius, entity, filter)
Physics.CheckOverlapSphereWithData(position, radius, entity)
Physics.CheckOverlapSphereWithData(position, radius, entity, filter)

Physics.TestCollision(entity)
```

### `RaycastHit`

| Field              | Type         |
| ------------------ | ------------ |
| `Hit`            | `bool`     |
| `CollisionPoint` | `Vector3f` |
| `SurfaceNormal`  | `Vector3f` |
| `Entity`         | `Entity`   |

### `Hit` (used inside `OverlapData`)

| Field      | Type         |
| ---------- | ------------ |
| `Entity` | `EntityID` |
| `Filter` | `Filter`   |

### `OverlapData`

| Field    | Type           |
| -------- | -------------- |
| `Hits` | List of`Hit` |

### `CollisionFilter`

A table of filter masks. Built-in entries:

- `CollisionFilter.Default`
- `CollisionFilter.All`

Any custom collision filter set up in the project's settings is exposed by name on this table
(e.g. `CollisionFilter.Player`, `CollisionFilter.Enemy`).

### `CollisionFilterManager`

Allows runtime querying / modification of project filter slots.

```lua
CollisionFilterManager.SetFilterNameAtSlot(slot, name)
CollisionFilterManager.GetFilter(name)        -- Filter
CollisionFilterManager.GetFilterNameBySlot(slot)
```

---

## Audio

### `AudioSystem`

```lua
AudioSystem.PlaySound("ExplosionSfx")                       -- one-shot, default props
AudioSystem.PlaySound("Music", props)                       -- with custom AudioSoundProperties
AudioSystem.PlaySound("Footstep", props, position)          -- spatialized at position (Vector3f)

AudioSystem.PlaySoundDelayed("ExplosionSfx", 250.0)         -- same overloads, delayed by milliseconds
AudioSystem.PlaySoundDelayed("Footstep", 250.0, props, position)
```

Every `PlaySound`/`PlaySoundDelayed` overload also accepts a clip `UUID` in place of the name.

`PlayOneShot` is the lightest-weight way to fire a sound — it takes a clip `UUID` only and creates
no entity, so it can't be stopped or repositioned once started:

```lua
AudioSystem.PlayOneShot(clipHandle)
AudioSystem.PlayOneShot(clipHandle, props)
AudioSystem.PlayOneShot(clipHandle, props, position)
```

One-shots are cut off when the scene stops, and their audio data is cached after the first play so
repeat fires don't re-read the file. Loading always happens off the game thread, so a clip that
isn't cached yet starts a few milliseconds late rather than stalling the frame. Whether a clip is
decoded into memory or streamed from disk is set per-asset in Ember-Forge (right-click the clip in
the Asset Manager → **Load Mode**); **Auto** decodes small files and streams large ones, which is
usually what you want — force **Stream** for music and **Decode** for a short effect fired rapidly.

### `AudioSoundProperties`

```lua
local props = AudioSoundProperties.new()
props.Volume      = 1.0
props.Pitch       = 1.0
props.Looping     = false
props.Spatialized = true
props.MinDistance = 1.0
props.MaxDistance = 50.0
```

---

## Assets

The asset manager exposes a handle-based fetch API:

```lua
local tex   = AssetManager.GetAsset("Texture",   "Crosshair")
local mesh  = AssetManager.GetAsset("Mesh",      "Sword")
local model = AssetManager.GetAsset("Model",     "PlayerModel")
local skel  = AssetManager.GetAsset("Skeleton",  "Humanoid")
local anim  = AssetManager.GetAsset("Animation", "Run")
local shdr  = AssetManager.GetAsset("Shader",    "StandardLit")
local mat   = AssetManager.GetAsset("Material",  "RedPlastic")
```

Returned asset types expose at minimum:

| Type          | Methods                                            |
| ------------- | -------------------------------------------------- |
| `Animation` | `:GetUUID()`, `:GetName()`, `:GetDuration()` |
| `Texture`   | `:GetUUID()`, `:GetName()`                     |

> Use the asset's `UUID` to assign it to component `*Handle` fields, e.g.
> `entity:GetComponent("SpriteRendererComponent").TextureHandle = tex:GetUUID()`.

> Script assets cannot be retrieved through this API.

---

## Particles

The global `Particles` table emits one-shot bursts from a `ParticleEmitterComponent`. This is useful
for impacts, pickups, muzzle flashes, and explosions where you do not want to keep an emitter entity
running continuously.

```lua
local emitter = entity:GetComponent("ParticleEmitterComponent")

Particles.Burst(emitter, entity:GetComponent("TransformComponent").WorldPosition, 24)
Particles.Burst(emitter, entity:GetComponent("TransformComponent").WorldPosition, 24, Quaternion.new())
```

---

## Save Game

Persistent key/value storage, accessed through the global `GameData`. Each **save file** is an
independent set of keys stored as one `.sav` under `%LOCALAPPDATA%\<ProjectName>\SavedGames`, and any
number of them can be open at the same time — a high-score table and a settings file never interfere.

`GameData` has no value accessors of its own. Every read and write goes through a file, so it is
always explicit which file a value belongs to.

### Opening a file

`GameData:Open(name)` returns the save file, reading `<name>.sav` from disk the first time it is
opened and starting empty if there is no such file. The `.sav` is optional — `Open("Settings")` and
`Open("Settings.sav")` are the same file.

```lua
local settings = GameData:Open("Settings")
local scores   = GameData:Open("HighScores")
```

The returned value is safe to cache. Open once in `OnCreate`, store it on `self`, and reuse it every
frame — that is cheaper than re-opening by name, and the value stays usable as other files are
opened and across `GameData:Reload`.

### Reading and writing values

A file holds four value types: **int**, **float**, **bool**, and **string**.

```lua
settings:SetFloat("Volume", 0.8)
settings:SetBool("Subtitles", true)
settings:SetString("Language", "en")
scores:SetInt("Best", 9001)

local volume = settings:GetFloat("Volume", 1.0)   -- 1.0 if "Volume" was never set
local best   = scores:GetInt("Best")              -- default is 0 / 0.0 / false / "" if omitted
```

The getters take an optional fallback returned when the key is missing. A key holds one value
regardless of type — `SetInt("X", 1)` followed by `SetString("X", "a")` leaves a string.

Reading a key as a different type than it was written converts where that is meaningful:

| Stored as | `GetInt` | `GetFloat` | `GetBool` | `GetString` |
|---|---|---|---|---|
| Int | value | value | non-zero → `true` | fallback |
| Float | truncated | value | fallback | fallback |
| Bool | `1` / `0` | fallback | value | fallback |
| String | fallback | fallback | fallback | value |

Int and float convert freely because YAML does not reliably distinguish `1` from `1.0` — a value
written with `SetFloat` still reads correctly through `GetInt` after a round trip through disk.

Full method list on a save file:

| Method | Returns | Notes |
|---|---|---|
| `SetInt` / `SetFloat` / `SetBool` / `SetString` `(key, value)` | — | |
| `GetInt` / `GetFloat` / `GetBool` / `GetString` `(key [, fallback])` | value | `fallback` when the key is missing |
| `Has(key)` | bool | |
| `Remove(key)` | bool | `true` if the key existed |
| `Clear()` | — | drops every key, leaves the file open |
| `Count()` | int | number of keys held |
| `GetName()` | string | the file's name, without the `.sav` |
| `IsValid()` | bool | `false` once the file has been closed |
| `Save()` | bool | writes this file to disk |

### Saving

Nothing reaches disk until you ask for it. Save the one file you touched, or every open file at once:

```lua
settings:Save()      -- true on success
GameData:SaveAll()   -- true only if EVERY open file wrote successfully
```

### Managing files

| Method | Returns | Notes |
|---|---|---|
| `GameData:Open(name)` | save file | opens it, reading from disk on first use |
| `GameData:Reload(name)` | save file | re-reads from disk, discarding unsaved changes; empties the file if there is nothing on disk |
| `GameData:SaveAll()` | bool | writes every open file |
| `GameData:Close(name)` | — | drops it from memory **without** saving |
| `GameData:CloseAll()` | — | |
| `GameData:IsOpen(name)` | bool | is it currently in memory |
| `GameData:ExistsOnDisk(name)` | bool | does `<name>.sav` exist |
| `GameData:DeleteFromDisk(name)` | bool | deletes the file; does **not** close the in-memory copy |

To wipe a save completely, close it as well as deleting it — otherwise the next `Open` hands back the
copy still held in memory:

```lua
GameData:DeleteFromDisk("Progress")
GameData:Close("Progress")
```

### Lifetime

Open files live as long as the game process, **not** the scene. Loading another scene keeps them
open with their values intact, which is how one scene hands progress to the next without touching
disk. In the editor they also survive leaving and re-entering Play mode, so a second Play session
sees whatever the first left in memory rather than a fresh read — use `GameData:Reload(name)` when
you specifically need what is on disk.

Closing a file invalidates every value `Open` returned for it. Calling a method on a closed file
raises a Lua error, so use `IsValid()` if your own code closed it:

```lua
GameData:Close("Settings")
settings:IsValid()          -- false
settings:GetFloat("Volume") -- error: file is no longer open
```

### Example

Two files, opened once and cached, with progress written as it changes and saved at a checkpoint:

```lua
local Player = {}

function Player:OnCreate(entity)
    self.progress = GameData:Open("Progress")
    self.settings = GameData:Open("Settings")

    self.volume = self.settings:GetFloat("Volume", 1.0)
    self.deaths = self.progress:GetInt("Deaths", 0)
end

function Player:OnDeath()
    self.deaths = self.deaths + 1
    self.progress:SetInt("Deaths", self.deaths)
end

function Player:OnCheckpoint(checkpointName)
    self.progress:SetString("Checkpoint", checkpointName)
    self.progress:Save()
end

return Player
```

---

## Debug Drawing

The `Debug` table draws primitives that show up in the editor's debug pass.

```lua
Debug.DrawLine(pointA, pointB)                          -- Vector3f, Vector3f
Debug.DrawLine(pointA, pointB, color)                   -- + Vector4f
Debug.DrawLine(origin, direction, length)
Debug.DrawLine(origin, direction, length, color)

Debug.DrawTriangle(a, b, c, color)
Debug.DrawCube(center, size, color)
Debug.DrawOctahedron(center, size, color)
```

---

## Standard Lua Libraries

The runtime opens the following sol2/Lua libraries:

- `base` — `print`, `pcall`, `tostring`, `type`, ...
- `math` — Lua's standard math library (`math.sqrt`, `math.atan`, ...)
- `string` — Lua string utilities (`string.format`, ...)
- `table` — Lua table utilities (`table.insert`, `table.remove`, ...)

> Other Lua standard libs (`io`, `os`, `package`, `debug`) are intentionally **not** loaded for
> sandboxing reasons. Use the engine's APIs (`Log`, `GameData`, `SceneManager`) instead.

---

## Worked Example: Top-down Player

```lua
local Player = {}

Player.Speed     = 6.0
Player.JumpBoost = 1.0

function Player:OnCreate(entity)
    self.controller = entity:GetComponent("CharacterControllerComponent")
    self.transform  = entity:GetComponent("TransformComponent")
    self.progress   = GameData:Open("Progress")
end

function Player:OnUpdate(entity, delta)
    local forward = self.transform:GetForward()
    local right   = self.transform:GetRight()

    local move = Vector3f.new(0, 0, 0)
    if Input.IsKeyDown(KeyCode.W) then move = move + forward end
    if Input.IsKeyDown(KeyCode.S) then move = move - forward end
    if Input.IsKeyDown(KeyCode.D) then move = move + right end
    if Input.IsKeyDown(KeyCode.A) then move = move - right end

    if Math.Length(move) > 0 then
        move = Math.Normalize(move)
    end

    self.controller:Move(move * self.Speed * delta)

    if Input.IsKeyDown(KeyCode.Space) and self.controller.IsGrounded then
        self.controller:Jump()
    end
end

function Player:OnOverlapTriggerEnter(entity, other)
    if other:GetName() == "Coin" then
        Scene.RemoveEntity(other)
        self.progress:SetInt("Score", self.progress:GetInt("Score") + 1)
    end
end

return Player
```
