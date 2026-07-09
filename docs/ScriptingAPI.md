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
supported type (`int`, `float`, `string`, `bool`, or an enum table — see
[Enum Properties](#enum-properties)) are surfaced as **editable properties** in the Inspector.
Any of the following methods, if present, are invoked by the engine:

| Method                                         | When called                                                    | Signature                                   |
| ---------------------------------------------- | -------------------------------------------------------------- | ------------------------------------------- |
| `OnCreate(self, entity)`                     | Once, when the script is first instantiated.                   | `entity: Entity`                          |
| `OnUpdate(self, entity, delta)`              | Every frame the entity is active.                              | `entity: Entity, delta: number` (seconds) |
| `OnOverlapTriggerEnter(self, entity, other)` | A trigger collider on`entity` started overlapping `other`. | `entity, other: Entity`                   |
| `OnOverlapTriggerStay(self, entity, other)`  | Continuous overlap with`other`.                              | `entity, other: Entity`                   |
| `OnOverlapTriggerExit(self, entity, other)`  | The overlap with`other` ended.                               | `entity, other: Entity`                   |

> The engine treats those five names as reserved lifecycle hooks. Any other field on the returned
> table is considered a *property* and is candidate for editor exposure.

### Minimal script template

```lua
local MyScript = {}

-- Editor-exposed properties (must be primitive types)
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

| Field     | Type         |
| --------- | ------------ |
| `Text`  | `string`   |
| `Color` | `Vector4f` |

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

---

## Input

```lua
Input.IsKeyPressed(KeyCode.W)             -- bool, this frame
Input.IsKeyHeld(KeyCode.Space)            -- bool, currently held
Input.IsMouseButtonPressed(MouseButton.Left)

Input.GetMousePosition()                  -- Vector2f
Input.GetMouseScrollOffset()              -- Vector2f
Input.GetMouseDelta()                     -- Vector2f, since last frame

Input.SetCursorMode(mode)
Input.GetCursorMode()
```

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
- **`KeyModifier`** — `None`, `Shift`, `Control`, `Alt`, `Super`.
- **`MouseButton`** — `Left`, `Right`, `Middle`.

`Input.SetCursorMode(mode)` accepts Ember's cursor mode value: `0` normal, `1` hidden, `2` locked.

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
```

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

A persistent key/value store, accessed through the global `GameData`:

```lua
GameData:SetInt("Score", 1500)
GameData:SetFloat("Volume", 0.8)
GameData:SetString("PlayerName", "Hero")

local score = GameData:GetInt("Score")
local vol   = GameData:GetFloat("Volume")
local name  = GameData:GetString("PlayerName")

GameData:Save()      -- writes to disk
GameData:Load()      -- reads from disk
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
end

function Player:OnUpdate(entity, delta)
    local forward = self.transform:GetForward()
    local right   = self.transform:GetRight()

    local move = Vector3f.new(0, 0, 0)
    if Input.IsKeyPressed(KeyCode.W) then move = move + forward end
    if Input.IsKeyPressed(KeyCode.S) then move = move - forward end
    if Input.IsKeyPressed(KeyCode.D) then move = move + right end
    if Input.IsKeyPressed(KeyCode.A) then move = move - right end

    if Math.Length(move) > 0 then
        move = Math.Normalize(move)
    end

    self.controller:Move(move * self.Speed * delta)

    if Input.IsKeyPressed(KeyCode.Space) and self.controller.IsGrounded then
        self.controller:Jump()
    end
end

function Player:OnOverlapTriggerEnter(entity, other)
    if other:GetName() == "Coin" then
        Scene.RemoveEntity(other)
        GameData:SetInt("Score", GameData:GetInt("Score") + 1)
    end
end

return Player
```
