# Phase 2: Game Implementation Review — ZombieFPS

## 0. Reality Check

Right now this is an **FPS sandbox**, not a zombies game. The shooter loop (move, aim, fire, recoil, reload, swap, pick up) is implemented; the *zombies* part — enemies, AI, navigation, waves, health, damage, scoring, round state — does not exist. `GameData/Assets/Scripts/` contains only `HUD/`, `Pickups/`, `Player/`, `Weapons/`. There is exactly one scene (`Default.ebs`), no `Enemies/`, no `AI/`, no `WaveManager`, no `Health`, no `Damage`, no scoring beyond stub `Log.Info` calls.

Half the friction you'll feel later is invisible until you start adding those systems, so this review calls out the load-bearing weaknesses now while they're cheap to fix.

---

## 1. Gameplay Architecture

### CRITICAL G1 — String-based singleton wiring is the entire architecture
Every weapon subsystem is a separate scene entity (`WeaponFire`, `WeaponRecoil`, `WeaponAiming`, `MuzzleFlash`, `WeaponHolder`, `BarrelTip`, `AmmoUI`, `PickupItemUI`, `Crosshair`, `ADS_Node`, `WeaponHandler`, `Player`, `Camera`) and they find each other by hard-coded `Scene.GetEntityByName("…")` calls. `WeaponFire:Fire` alone does **four** `GetEntityByName` lookups per shot (`WeaponAiming`, `MuzzleFlash`, `BarrelTip`, `WeaponRecoil`). `WeaponHolder:OnShoot` looks up the current weapon's entity by name even though the script already holds the entity reference (`self.Weapons[self.ActiveWeaponSlot]` is the entity — it then re-fetches it by `GetName()`). `WeaponMovement:Bob` calls `Scene.GetEntityByName("Player")` *every frame*.

This is a string-typed service locator masquerading as architecture. Any rename in the editor silently breaks runtime; there's no compile-time check; and it scales linearly per-frame with the number of cross-script reaches.

**Fix:** cache entity handles in `OnCreate` (as `WeaponAiming` and `WeaponRecoil` correctly do), or expose them as editor-assignable script properties so designers wire them once. The ones that *can't* be cached at create time (the equipped weapon entity) should be passed as arguments through method calls — `WeaponHolder` already has the handle, just hand it to `WeaponFire:Fire(weaponEntity)` and `WeaponRecoil:Fire(weaponEntity)`.

### CRITICAL G2 — There is no weapon state machine
You used the words "weapon state machines" but there isn't one. State is scattered across booleans on different scripts:
- `WeaponController.IsReloading`, `CanShoot`, `ReloadTimer` (unused — animation events drive it)
- `WeaponFire.CanShoot`, `TimeSinceLastShot`
- `WeaponHolder.PlayedEmptyGunSound`, `ActiveWeaponSlot`
- `WeaponAiming.IsAiming`

Nothing prevents reloading while ADS-transitioning, firing during a weapon swap, swapping during reload, or firing on the same tick `IsReloading` flips. There's no Idle / Firing / Reloading / Swapping / Inspecting / Dead enum, and no central tick that resolves which transitions are legal. The reload-via-animation-events approach (`MagOut` / `MagIn` / `RefillAmmo` / `ReloadCompleteEventName`) is correct in spirit but means the *only* way to cancel a reload safely is to play another animation — there's no state owner that can say "ignore the next `RefillAmmo` event because we swapped weapons."

**Fix:** one `WeaponFSM` Lua module owning `state ∈ {Idle, Firing, Reloading, Swapping, Empty}` with explicit transitions. Every input handler calls `fsm:CanFire()` / `fsm:CanReload()` instead of poking flags directly. Animation events route through the FSM, not through `WeaponController` — it should ignore stale events when state ≠ Reloading.

### CRITICAL G3 — Raycast hit detection is direction-of-the-gun, not direction-of-the-camera
`WeaponFire:Fire` uses `transform:GetForward()` of the **WeaponFire entity** as the ray direction. If WeaponFire is parented under the gun model — which it appears to be by the architecture — every recoil rotation, sway, bob, and ADS-lerp on the gun mesh deflects bullets. You have a glaringly visible "the gun jiggles, bullets miss" bug waiting to happen.

A hitscan shooter must raycast from the **camera** through the **screen-center reticle**, not from the visual gun. The visual tracer can spawn from `BarrelTip`, but the hit math has to come from the camera. The crosshair `Crosshair.lua` is locked to (0.5, 0.5) — that *is* the camera-forward intersection at z = far. Use it.

**Fix:**
```lua
local cam = self.CameraEntity:GetComponent("TransformComponent")
local origin = cam.WorldPosition
local dir    = cam:GetForward()
-- apply bloom in camera basis, not gun basis
```

### CRITICAL G4 — `Scene.GetEntityByName` returning stale references
`WeaponHolder:OnShoot` does:
```lua
local weaponEntity = Scene.GetEntityByName(self:GetCurrentWeapon():GetName())
```
If two prefab instances share a name (e.g. attachments named `Sight`, `Muzzle`), name lookups hit the *first* match. Worse, `WeaponHolder:EquipWeapon` slot-juggling logic also hard-codes name lookups; if you ever equip two pistols of the same prefab, the system silently aliases them.

Use the entity handle directly. `self:GetCurrentWeapon()` already returns it.

### G5 — Inventory is a fixed `{nil, nil}` array with subtle bugs
`WeaponHolder.WeaponSlots = 2` but the array is hardcoded `{nil, nil}` in `OnCreate`. Any change to `WeaponSlots` in the editor desyncs the array vs the loop bound. `EquipWeapon`'s "move to empty slot" logic doesn't update `ActiveWeaponSlot` after the move, so the active pointer can end up referencing the *moved-from* slot which is now nil — caller will then re-equip into the empty slot it just vacated. There's no slot-cycling input (Q / 1 / 2 / scroll wheel) implemented at all, so the second slot is unreachable in-game today.

**Fix:** initialize `self.Weapons = {}` for `i = 1, self.WeaponSlots do self.Weapons[i] = nil end`. Add `:SwitchSlot(idx)` and bind to keys. After moves, set `ActiveWeaponSlot = newSlot`.

### G6 — `PowerUpDrop.AddAmmo` references a field that doesn't exist
```lua
if weaponHolderScript and weaponHolderScript.CurrentWeapon ~= "" then
    local weaponEntity = Scene.GetEntityByName(weaponHolderScript.CurrentWeapon)
```
`WeaponHolder` has no `CurrentWeapon` field — current weapon is `self.Weapons[self.ActiveWeaponSlot]`, an entity, not a string. Ammo pickups are silently no-ops. Dead code path.

### G7 — `Tracer.lua` divides by zero, and tracer speed is wrong
`self.TotalDistance = Math.Distance(startPos, endPos)`. If a shot hits at point-blank (distance ≈ 0), `progress = self.DistanceTraveled / 0 = inf`, the tracer is destroyed on its first frame, fine — but if `startPos == endPos` exactly, NaN propagation through `Math.Lerp` will write NaN into the transform. Guard with `if self.TotalDistance < 0.05 then Scene.RemoveEntity(entity); return end`.

Also `Speed = 100 m/s` for a bullet is roughly an arrow. Real tracers fly at 800–1000 m/s; visually this needs to be at least 300–400 m/s or the tracer streak lags badly behind impact effects.

### G8 — Tracer not pooled; impacts are
`WeaponFire` correctly uses `Scene.RetrieveFromPool("ImpactConcretePool", ...)` for impacts but spawns tracers with `Scene.InstantiatePrefab("Tracer", ...)` and destroys them on completion. At 600 RPM that's 10 entity create/destroy + script instance allocations per second per firing weapon. Add a `TracerPool` and reuse.

### G9 — `PowerUpDrop` mutates `transform.Position.y` directly
```lua
transform.Position.y = self.StartY + ...
```
This is the classic Sol2 / glm value-vs-reference trap. If `Position` is bound as a copy (very common in custom engines), this writes to a temporary that gets discarded. If it's bound by reference, it works. The Phase 1 review already flagged that the engine returns raw component pointers; here the question is whether `Vector3f` is a value type. Worth verifying — the bobbing motion should be visible at runtime if it works. If it doesn't, you have to write `transform.Position = Vector3f.new(transform.Position.x, newY, transform.Position.z)`.

The same suspicion applies to every `transform.Rotation.y = …` in `MouseLook`, `WeaponRecoil`, and `PowerUpDrop`.

### G10 — Are scripts decoupled from the engine core?
**No, in two distinct ways:**

1. *Coupled to engine internals via raw component pointers.* `MuzzleFlash.lua` literally has a code comment warning that caching component handles is unsafe because "spawning a prefab that adds the same component type can reallocate the pool and invalidate cached pointers." That is a script worked around an engine bug. Phase 1's C4 / F6 (handle-based component binding) is exactly the fix.

2. *Coupled to scene-tree topology by string.* The whole script ecosystem assumes a specific named entity hierarchy: `Player → Camera → WeaponHandler → WeaponFire / WeaponRecoil / WeaponAiming / MuzzleFlash / BarrelTip / WeaponHolder → <Gun> → <MountPoints>`. You cannot reuse `WeaponFire.lua` in another scene without recreating that exact tree with exact names. That's the opposite of decoupled.

The shape of decoupling is fine — gameplay in Lua, engine in C++ — but the **interface** between them is brittle: strings, raw pointers, sibling-by-name. Make the boundary contract explicit (handle-based, signal-based) and the rest follows.

---

## 2. Zombie / AI Systems — *Forecast*

There is no zombie code. So instead of reviewing what isn't there, here's where the current architecture will fail when it scales to 50+ enemies:

### Z1 — `Scene.GetEntityByName` will be the AI hot path
If each zombie does what player scripts do today — look up the player, the WeaponFire, the navmesh, the spawn manager by name each tick — at 50 zombies × 60 Hz × 4 lookups = 12 000 hash lookups per second just for wiring, before any actual AI runs. The fix isn't in Lua, it's in the engine: provide tag-based queries (`Scene.FindByTag("Player")`) that cache, and a global `Game` table for singletons set once at scene start.

### Z2 — Lua-per-zombie OnUpdate will not scale
Phase 1 noted `ScriptSystem` invokes Lua per-entity per-frame with full Sol2 marshalling. 50 zombies each running a behavior tree in Lua with raycasts (line-of-sight), distance checks, and animation parameter writes is going to dominate frame time long before the renderer does. Two options, in increasing engine cost:
- **Cheap:** tick AI at 10 Hz, not 60 Hz. Add `Script.SetTickRate(self, 0.1)` to the engine. Pathing decisions, target acquisition, and state-machine evaluations don't need 60 Hz.
- **Right:** native C++ AI components (perception, navigation agent, blackboard) driven by a small declarative behavior-tree asset that Lua only authors. Lua becomes the editor-time DSL, not the runtime executor.

### Z3 — There is no NavMesh
`PlayerInteraction` raycasts for pickups; that's the only spatial query in the codebase. Zombies cannot pathfind around a corner with raycasts alone. You cannot ship a Zombies clone without navmesh-based pathfinding. This is **F1** from Phase 1 (Recast/Detour). Everything else in AI — flow-field swarming, off-mesh links for window vaults, dynamic obstacle avoidance for barricades — depends on it. Ship-blocker.

### Z4 — There is no perception system
Target acquisition for a wave shooter is conceptually simple ("the player is the target") but the moment you add multiple players (co-op), distractions (claymore decoys, monkey bombs in CoD-Z parlance), or stunned/downed states, you need a **Perception** component (vision cone, hearing radius, last-known-position) and a **Threat** evaluator. Build it as a C++ component now while the design surface is small.

### Z5 — Wave management is a system-design hole, not a script hole
Zombies-clone wave logic isn't "spawn N every T seconds." It's:
- Round number → enemy HP curve, speed curve, count cap, special-enemy probability table
- Active-zombie cap (typically 24 in CoD-Z) with a spawn queue
- Spawn-point selection by player line-of-sight rejection (don't spawn in front of player)
- Round transitions, dog rounds, boss rounds, power-up drop tables tied to kill streaks

This needs a **GameMode / GameState** system. Today there's no concept of game state at all — no `GameStateComponent`, no round, no score, no game-over screen routing. Build this as a Lua module backed by a C++ singleton service (`Game.GetMode()`) so it's accessible from anywhere without `GetEntityByName`.

### Z6 — Spatial queries you don't have
Once 50 zombies exist, the player will need radius queries ("nearest 5 zombies for grenade splash"), the AI will need overlap queries ("are any allies in my swing arc"), and the wave manager will need spawn-point line-of-sight tests. ReactPhysics3D supports all of this; expose `Physics.OverlapSphere`, `Physics.SphereCast`, `Physics.LineOfSight` to Lua.

---

## 3. Engine-to-Game Friction Points

Each item below is a specific engine upgrade justified by a current Lua workaround.

| # | Friction (in script today) | Required engine upgrade |
|---|---|---|
| **E1** | `MuzzleFlash.lua` has a defensive comment forbidding caching component handles because pool reallocation invalidates them. Every script re-fetches components every frame. | Phase 1 **F6** — handle-based component binding (`ComponentRef<T>`) that tolerates pool moves. This single change cuts per-frame `GetComponent` hash lookups across the entire game by ~80%. |
| **E2** | Cross-script comms via `Scene.GetEntityByName(...):GetScriptInstance(...)`. Brittle, slow, untyped. | **Signal/event bus** in the engine: `Events.Publish("WeaponFired", payload)`, `Events.Subscribe(self, "WeaponFired", handler)`. Decouples WeaponFire ↔ Recoil ↔ MuzzleFlash ↔ AmmoUI completely. Also unblocks decoupled gameplay-to-UI flow (damage events, score events, round events). |
| **E3** | Designers can't wire entity references in the inspector, so scripts hardcode names (`"WeaponAiming"`, `"BarrelTip"`, `"Camera"`). | **Entity-typed script properties** — let a Lua script declare `WeaponController.AmmoUIRef = Entity` and surface that as a drag-target slot in Forge. Eliminates `GetEntityByName` for design-time wiring. |
| **E4** | `PickupItem:OnCreate` instantiates a child prefab to provide its visual, because there's no way to author the visual + collider + script as one unit in the editor. | **Nested prefab / prefab variants** support so a pickup is one authored asset, no runtime child instantiation. |
| **E5** | `WeaponController:OnAnimationEvent` is the *only* way reload timing works. There's no fallback if the animation doesn't fire `RefillAmmo` (e.g. anim-graph misconfigured). | **Animation-event introspection + safety timeout** in the C++ animator — emit a `MissedEvent` after timeline end if the named event was never reached, and let scripts handle it. Saves a class of soft-locks. |
| **E6** | No damage / health primitive. When zombies arrive, *every* one of you ad-hoc'd a `health` field per script, with no consistent damage source/type/team filtering. | Native **`HealthComponent`** + **`DamageEvent`** emitted by the engine when raycasts/overlaps tagged as weapons hit colliders with health. Bind both to Lua. Also unlocks editor tooling (damage previews, hit feedback). |
| **E7** | `WeaponFire` has to know about `Particles.Burst`, `RetrieveFromPool`, `Math.LookAt`, `SurfaceNormal` z-fighting offset, etc. — a "decal/hit-effect" composition the script reinvents every shot. | **Hit-effect descriptor asset** (`.ebimpact`): per-surface-tag mapping → particle prefab + sound + decal. Engine resolves which to spawn from a hit's surface tag. Script just calls `Effects.SpawnImpact(hitResult)`. Surface-typed audio falls out for free. |
| **E8** | `Crosshair.lua` recomputes its NDC scale every frame from `Renderer.GetViewportSize()`. Sign of no anchor/UI-layout system. | **Anchor / pivot-based UI layout** in the engine (top-left / center / bottom-right with pixel offsets). Removes a class of per-frame layout math and the existing breakage on aspect-ratio change. |
| **E9** | `WeaponHolder` swap logic uses string lookups + name comparisons. There's no sense of "this entity belongs to slot 1 of weapon holder" in the engine. | **Slot/socket attachment API** on the engine: `Entity:AttachTo(parent, "RightHandSocket")`. Combined with **bone sockets** (which Phase 1 noted you have partial support for) this is the ergonomic fix. |
| **E10** | No `Math.RandomFloat` seed control visible; recoil is non-deterministic. Replays/desync-prone if you ever go MP. | **Deterministic RNG service** with per-stream seeding (`Random.NewStream("recoil", seed)`). |
| **E11** | (Forecasted) AI raycasts will route through `Physics.CastRay` per zombie per frame. | **Batched async raycast API** (`Physics.CastRayBatch(jobs) → results next frame`). React supports this pattern; expose it to Lua. |
| **E12** | (Forecasted) No save system. Wave shooters need round/score/loadout persistence. | **Serialization service** with explicit `Serialize`/`Deserialize` Lua hooks per script, backed by a binary schema (rapidyaml is already in vendor — sufficient). |

---

## Bottom Line

**Phase 1 said the engine is structurally sound but has bottlenecks.** Phase 2 is more pointed: **the game code is held together by string lookups and load-bearing global entity names**, with no formal state machines, the wrong raycast origin, an unfilled inventory abstraction, and an entire AI/wave/health/damage hemisphere that does not exist yet. None of this is hard to fix because there isn't much code — but the engine upgrades in §3 (handles, events, entity-typed properties, health/damage primitives, navmesh) gate most of it. Build E1, E2, E6, and F1 (Phase 1 NavMesh) before you write a single line of zombie AI; otherwise the AI code will inherit every brittleness in the player code and multiply it by 50.
