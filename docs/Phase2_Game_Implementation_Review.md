# Phase 2: ZombieFPS Game Implementation Architecture Review

Project reviewed: `C:\Development\Games\ZombieFPS`

This review covers the Lua gameplay scripts and game-authored Ember files for ZombieFPS, including the asset registry, scene file, prefabs, pickups, HUD scripts, weapon scripts, and project metadata.

Inspected game files include:

- `ZombieFPSGame/ZombieFPS.ebproj`
- `ZombieFPSGame/GameData/Assets/Assets.eba`
- `ZombieFPSGame/GameData/Scenes/Default.ebs`
- `ZombieFPSGame/GameData/Assets/Scripts/HUD/*.lua`
- `ZombieFPSGame/GameData/Assets/Scripts/Pickups/*.lua`
- `ZombieFPSGame/GameData/Assets/Scripts/Player/*.lua`
- `ZombieFPSGame/GameData/Assets/Scripts/Weapons/*.lua`
- `ZombieFPSGame/GameData/Assets/Prefabs/*.ebprefab`

The short version: ZombieFPS is currently a weapon, pickup, and player interaction prototype. It is not yet architected as a zombie wave shooter. The current code can support a single-player firing range, but it will not scale cleanly to 50+ enemies, multiple weapons, multiple zombie archetypes, wave pacing, or robust content iteration without engine and gameplay architecture changes.

## Critical Findings

### 1. There Is No Zombie Or Wave System Yet

The most important finding is not a bug inside the zombie system. The problem is that the zombie system does not exist in the current game implementation.

The project metadata defines an `Enemy` collision filter in `ZombieFPS.ebproj`, but the inspected scripts, scene, prefabs, and asset registry do not contain zombie, wave, spawner, target-acquisition, health, damage, or AI controller gameplay assets.

No authored usage was found for:

- `Zombie`
- `Wave`
- `Spawner`
- `AIAgentComponent`
- `AIPathComponent`
- `NavigationGridComponent`
- `WaypointComponent`

The current `Default.ebs` scene contains player, weapon, pickup, UI, environment, and effect-pool entities. It does not contain zombie spawn points, zombie prefabs, AI navigation data, wave state, or enemy management entities.

This is a critical architecture gap because a zombie FPS lives or dies on enemy pacing, spawn budgets, navigation behavior, target selection, damage flow, and death/drop loops. None of those have a durable owner yet.

Recommended direction:

- Add a native `WaveDirectorSystem` or equivalent gameplay system.
- Add `SpawnPointComponent`, `ZombieAgentComponent`, `HealthComponent`, and `DamageableComponent`.
- Keep high-volume zombie update logic in C++ ECS systems.
- Use Lua only for wave tuning, events, special zombie variants, and one-off behaviors.

### 2. Weapon State Is Split Across Scripts Without A Real State Machine

Weapon behavior is split across several scripts:

- `Weapons/WeaponHolder.lua`
- `Weapons/WeaponController.lua`
- `Weapons/WeaponFire.lua`
- `Weapons/WeaponAiming.lua`
- `Weapons/WeaponRecoil.lua`
- `Weapons/WeaponMovement.lua`
- `Weapons/MuzzleFlash.lua`
- `Weapons/Tracer.lua`

The conceptual separation is reasonable, but state ownership is fragmented.

`WeaponHolder:OnUpdate` owns input routing. `WeaponHolder:OnShoot` resolves the current weapon and calls the weapon controller. `WeaponController` owns ammo and reload state. `WeaponFire:Fire` owns rate of fire, spread, raycast, impact effects, tracers, muzzle flash, and recoil dispatch. `WeaponAiming`, `WeaponMovement`, and `WeaponRecoil` mutate viewmodel and camera state separately.

There is no canonical state enum such as:

- `Idle`
- `Firing`
- `Reloading`
- `Equipping`
- `Switching`
- `Blocked`

As a result, fire and reload gating are fragile. `WeaponController:OnReload` sets `IsReloading = true`, but does not clearly make the weapon unable to shoot. `WeaponHolder:OnShoot` checks `weaponControllerScript.CanShoot`, but not `IsReloading`. This makes shooting during reload likely unless some unrelated animation event or side effect prevents it.

The reload ammo math is also incorrect for partial magazines. In `WeaponController:OnAnimationEvent`, the `MagIn` branch refills ammo using reserve ammo in a way that replaces current magazine ammo instead of topping it off. For example, a weapon with 25 rounds in the magazine and 10 in reserve should end at 30 in the magazine and 5 in reserve. The current logic can produce 10 in the magazine and 0 reserve.

Recommended direction:

- Introduce a single weapon runtime state object.
- Make fire, reload, equip, switch, and ADS transitions explicit.
- Move ammo math into one tested function.
- Treat animation events as notifications into the state machine, not as the owner of weapon correctness.

### 3. Inventory Stores Entity Handles But Still Relies On Names

`WeaponHolder.lua` uses `self.Weapons = {nil, nil}` and `self.ActiveWeaponSlot`, which is a reasonable early prototype. The problem is that it stores entity handles but then repeatedly falls back to global name lookup.

For example, `WeaponHolder:OnShoot` resolves the current weapon through the current weapon entity name, then calls `Scene.GetEntityByName`. This is brittle if there are multiple instances of the same weapon prefab, duplicate entity names, delayed removals, dropped weapons, or future multiplayer support.

There is also a data contract mismatch between scene-authored properties and script runtime state. The scene sets `CurrentWeapon` on the `WeaponHolder` script, but the script uses `self.Weapons` and `GetCurrentWeapon()`. `PowerUpDrop:AddAmmo` still tries to use `weaponHolderScript.CurrentWeapon`, which does not match the runtime inventory model.

This means the ammo power-up path is likely stale or broken once the current weapon is equipped through the actual inventory flow.

Recommended direction:

- Replace raw Lua arrays with explicit weapon slot records.
- Store stable entity references or runtime weapon IDs, not names.
- Make `WeaponHolder:GetCurrentWeapon()` the only public path for current weapon access.
- Update power-ups to call a clear inventory API such as `AddReserveAmmo(ammoType, amount)`.

### 4. Hitscan Has No Damage Contract

`WeaponFire.lua` defines a `Damage` property, but `WeaponFire:Fire` does not apply damage to anything.

The current implementation performs a raycast, optionally applies impulse to rigid bodies, spawns impact visuals, plays muzzle flash, spawns a tracer, and applies recoil. That is useful feedback logic, but it is not a gameplay combat pipeline.

Missing pieces:

- No `HealthComponent`.
- No `DamageableComponent`.
- No `DamageEvent` or damage payload.
- No hit group or weak-point data.
- No faction/team filter.
- No surface material response.
- No explicit enemy target interface.
- No collision filtering in the weapon raycast.

For a zombie FPS, weapons need to produce structured hit results, not just impact effects. A shot should be able to determine whether it hit environment, physics debris, an enemy body, an enemy head, a trigger, or a shield/armor surface.

Recommended direction:

- Add a native `HitResult` returned from raycasts.
- Add `HealthComponent` and `DamageableComponent`.
- Add `ApplyDamage(entity, damageEvent)` as a script-facing API.
- Include surface type, hit group, normal, collider entity, rigidbody entity, and collision category in raycast results.

### 5. Gameplay Scripts Are Over-Coupled To Global Scene Names

Many scripts communicate by global entity names rather than serialized references, prefab-local references, or system-owned services.

Examples:

- `WeaponFire.lua` looks up `WeaponAiming`, `MuzzleFlash`, `WeaponRecoil`, and `BarrelTip`.
- `WeaponAiming.lua` looks up `ADS_Node`, `Crosshair`, and `WeaponHandler`.
- `WeaponMovement.lua` looks up `Player` every frame in `Bob`.
- `PlayerInteraction.lua` looks up `PickupItemUI` and `WeaponHolder`.
- `WeaponController.lua` looks up `AmmoUI` and mount point names such as `ScopeMount`.
- `PickupWeapon.lua` finds `WeaponHolder` by name.

This is one of the main architecture limits in the current game. The scripts are not decoupled from engine core. They are coupled to global scene names, prefab tag names, script names, and singleton-like engine tables.

The failure mode is predictable: the first time there are two weapons, two players, two cameras, two weapon holders, or two copies of a prefab with the same child names, the code becomes ambiguous.

Recommended direction:

- Add serialized `EntityRef` script fields.
- Add prefab-local child lookup by path or UUID.
- Add weapon sockets instead of global mount point names.
- Add editor validation for missing references.
- Reserve `Scene.GetEntityByName` for debugging and prototypes, not production gameplay paths.

### 6. Unsafe Lua Component Lifetime Leaks Into Gameplay Code

`Weapons/MuzzleFlash.lua` contains an explicit warning not to cache `ParticleEmitterComponent` or `PointLightComponent` handles because `GetComponent` returns raw pointers into ECS storage and prefab spawning can invalidate them.

That comment is technically correct, and it is a serious engine-to-game friction point.

Gameplay scripts should not need to know that component storage can reallocate. Lua should not be handed raw component pointers whose lifetime can silently expire. This is exactly the kind of engine implementation detail that should be hidden behind safe handles or short-lived accessors.

Recommended direction:

- Replace raw Lua component pointers with generation-checked component proxies.
- Make component access fail loudly if the entity or component generation is stale.
- Encourage short-lived access patterns through script APIs.
- Avoid allowing Lua scripts to retain direct references into movable C++ component arrays.

## Gameplay Architecture Review

### Current Shape

ZombieFPS currently has these implemented gameplay pillars:

- First-person movement through `Player/CharacterMovement.lua`.
- Mouse look through `Player/MouseLook.lua`.
- Pickup interaction through `Player/PlayerInteraction.lua`.
- Weapon inventory and input routing through `Weapons/WeaponHolder.lua`.
- Weapon ammo, reload, attachments, animation events, and UI binding through `Weapons/WeaponController.lua`.
- Hitscan firing and visual effects through `Weapons/WeaponFire.lua`.
- ADS, recoil, sway, and bob through separate weapon scripts.
- Basic ammo UI through `HUD/AmmoUI.lua`.
- Crosshair behavior through `HUD/Crosshair.lua`.
- Ammo and attachment pickups through `Pickups/*.lua`.

This is a solid prototype slice for proving that the engine can support FPS mechanics. It is not yet a sustainable gameplay framework.

### Main Architectural Issue

The gameplay code is organized by visible behavior rather than durable ownership.

For example, firing a weapon currently crosses these responsibilities:

1. `WeaponHolder:OnUpdate` receives input.
2. `WeaponHolder:OnShoot` resolves the current weapon.
3. `WeaponController:OnShoot` decrements ammo.
4. `WeaponFire:Fire` executes raycast, spread, audio, tracer, impact effect, muzzle flash, and recoil.
5. `WeaponRecoil:Fire` mutates weapon and camera recoil.
6. `AmmoUI:SetAmmo` updates UI text.

That chain works while there is one player, one weapon handler, and one gun. It becomes fragile once you add weapon switching, different ammo types, enemy damage, animation cancel rules, pickups dropped by enemies, or save/load persistence.

The missing abstraction is a real gameplay domain model:

- Weapon definition data.
- Weapon runtime state.
- Inventory state.
- Ammo reserve state.
- Damage event model.
- Hit result model.
- Player combat controller.
- Enemy health/death/drop loop.

Lua can still participate, but it should sit on top of these concepts rather than inventing them ad hoc through entity names and script probes.

### Pickup Architecture

`PlayerInteraction.lua` raycasts against `CollisionFilter.PickupItem`, shows or hides `PickupItemUI`, then probes scripts such as `PickupItem` and `PickupWeapon` to decide what to do.

This is flexible but informal. Pickups should expose a unified interaction contract. The player interaction script should not care whether the target is a weapon pickup, attachment pickup, ammo box, door, button, or objective item.

Recommended direction:

- Add an `InteractableComponent` or script interface.
- Store prompt text, interaction range, and allowed input on the target.
- Let the target handle `OnInteract(interactorEntity)`.
- Avoid probing multiple script names from the player script.

### UI Architecture

`AmmoUI.lua` and `Crosshair.lua` are both simple and understandable, but they reveal UI system friction.

`AmmoUI` finds `AmmoText` by name and mutates `TextComponent.Text`. `Crosshair` manually reads viewport size and computes scale/position. These are jobs the UI system should largely own through anchors, layout rules, canvas scaling, and event-driven updates.

Recommended direction:

- Use serialized text references for UI bindings.
- Add viewport resize events.
- Add screen-space scaling rules that do not require per-frame Lua math.
- Make weapon/ammo UI subscribe to weapon state changes instead of being called manually from multiple scripts.

## Zombie And AI Systems Review

### Current State

No zombie gameplay system exists in the inspected project files.

There are no zombie prefabs, zombie scripts, wave scripts, spawn points, navigation components, AI agent components, health components, or damage handling scripts. The asset registry in `Assets.eba` registers weapon, pickup, HUD, and player scripts only.

The current game therefore cannot be evaluated as an implemented zombie system. It can only be evaluated for readiness to host one.

### Readiness For 50+ Enemies

The current scripting style should not be used as the base for 50+ zombie agents.

If implemented naively in the current style, each zombie would likely perform some combination of:

- Lua `OnUpdate` execution every frame.
- `Scene.GetEntityByName("Player")` lookups.
- Per-agent raycasts or distance checks.
- Per-agent movement decisions.
- Per-agent animation and attack state updates.
- Script-to-script calls for health, damage, and drops.

That will become difficult to profile, difficult to batch, and difficult to optimize. It also pushes deterministic gameplay, memory lifetime, pathfinding, animation, and physics decisions into Lua, which is the wrong layer for a high-count enemy simulation.

Recommended architecture:

- Use C++ ECS for zombie movement, targeting, path following, attack checks, health, death, and despawn.
- Use Lua for wave definitions, special events, boss modifiers, and content-specific callbacks.
- Keep shared player target data in a native blackboard or combat query service.
- Batch AI updates by distance, visibility, and threat state.
- Add AI LOD so distant zombies update less frequently.
- Avoid per-zombie path recomputation every frame.

### Wave Director Requirements

A proper wave director should own:

- Current wave index.
- Spawn budget.
- Alive enemy count.
- Pending enemy count.
- Spawn cooldowns.
- Difficulty scaling.
- Zombie archetype selection.
- Spawn point selection.
- Max active enemy cap.
- End-of-wave rewards.
- Drop chance tables.
- Player fail/success state.

This should not be scattered across individual zombie scripts. A zombie should not decide global pacing. The director should push spawn requests into the engine, and zombie agents should report death/despawn events back.

### Damage And Death Loop Requirements

For ZombieFPS to become an actual survival shooter, the combat loop needs a clean damage pipeline:

1. Weapon fires.
2. Raycast returns structured `HitResult`.
3. Hit result identifies damageable target and hit group.
4. Weapon builds `DamageEvent`.
5. Damage system applies health changes.
6. Death system emits death event.
7. Wave director updates alive count.
8. Drop system optionally spawns ammo, points, or power-up.
9. UI/audio/VFX systems react.

The current project has step 1 and part of step 2 for physics impacts. It does not yet have steps 3 through 9.

## Engine-To-Game Friction

### Missing Serializable References

The largest source of friction is the absence of robust serialized references in script properties.

Current scripts use strings for:

- Entity names.
- Prefab names.
- Mount names.
- Audio clip names.
- Script names.
- UI target names.

That keeps early iteration fast, but it makes content fragile. Renaming an entity can break gameplay silently. Duplicating a prefab can create ambiguous lookups. Moving a mount point can break attachments. Adding a second player can break almost every global lookup.

Engine upgrade:

- Add typed script properties for `EntityRef`, `PrefabRef`, `AudioClipRef`, `MaterialRef`, `CollisionFilterRef`, and `SocketRef`.
- Add editor pickers for those references.
- Add validation warnings for missing or broken references.
- Add prefab-local reference resolution.

### Missing Prefab-Local Entity Lookup

Weapon attachments currently rely on mount names such as `ScopeMount`. `WeaponFire` also depends on child entities such as `BarrelTip` and `MuzzleFlash`.

These should be resolved relative to the owning weapon prefab instance, not globally through the scene.

Engine upgrade:

- Add `entity:FindChild("Path/To/Child")` or UUID-backed child references.
- Add socket support for weapon models.
- Let prefabs expose named attachment points as data.

### Lua API Exposes Engine Internals Too Directly

The `MuzzleFlash.lua` component caching warning shows that Lua scripts are aware of ECS storage invalidation. That is a boundary leak.

Engine upgrade:

- Use safe component handles.
- Validate generation on access.
- Prefer short-lived borrowed component access.
- Do not expose raw pointers into vector-backed component storage as persistent Lua objects.

### Physics Queries Are Too Low-Level For Gameplay Combat

`Physics.CastRay` appears to be useful, but the weapon script needs a more gameplay-aware query.

Engine upgrade:

- Add collision filter masks to script raycasts.
- Return rich hit data.
- Distinguish trigger hits, rigidbody hits, collider hits, and damageable hits.
- Include surface/material data for impact effects.
- Support ignored entities, such as the player and equipped weapon.

### UI Layout Requires Too Much Script Logic

`Crosshair.lua` doing viewport math is a sign that the UI system is not carrying enough layout responsibility.

Engine upgrade:

- Add canvas scaling modes.
- Add resize events.
- Add center-anchored fixed-size elements.
- Make common HUD primitives require no per-frame script positioning.

### Effect Pooling Is Partial

The scene contains `ImpactConcretePool`, and `WeaponFire` uses `Scene.RetrieveFromPool("ImpactConcretePool")` for impact effects. That is good.

However, `Tracer.lua` is still spawned as a prefab and removed after lifetime completion. Automatic weapons can create many tracers quickly, so this should be pooled too.

Engine upgrade:

- Add generic pooled effect spawning.
- Add reusable pooled lifetime behavior.
- Let content declare an effect prefab as pooled in asset metadata.
- Avoid gameplay scripts having to manually choose between instantiate/remove and retrieve/return.

## Recommended Implementation Order

1. Fix weapon correctness first.
   - Add a real weapon state machine.
   - Fix reload ammo math.
   - Prevent fire during reload and switching.
   - Make ammo UI state-driven.

2. Add damage and health before zombies.
   - Add `HealthComponent`.
   - Add `DamageableComponent`.
   - Add `DamageEvent`.
   - Make `WeaponFire.Damage` actually apply to targets.

3. Replace global name lookups with references.
   - Add serialized entity/prefab/audio refs.
   - Add prefab-local child refs.
   - Convert weapon, UI, pickup, and mount scripts away from global names.

4. Build zombie systems natively.
   - Add zombie prefab data.
   - Add spawn points.
   - Add C++ zombie movement/targeting/attack systems.
   - Add wave director and active enemy budget.

5. Add AI scalability features.
   - AI LOD.
   - Batched target queries.
   - Path update throttling.
   - Native death/despawn/drop events.

## Bottom Line

ZombieFPS currently proves that Ember can support a first-person weapon prototype with pickups, UI, raycasts, animation events, particles, and prefab instantiation. That is valuable.

But the current game architecture is not yet a zombie FPS architecture. The survival-shooter-specific systems are absent, and the implemented systems rely heavily on global names, informal script contracts, raw component access, and scattered state ownership.

Before building the zombie loop, the project needs a small set of native gameplay foundations: references, damage, health, weapon state, prefab-local sockets, pooling, and wave/AI systems. Without those, every new gameplay feature will add more string-based glue and make the codebase harder to scale.