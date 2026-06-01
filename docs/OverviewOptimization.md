# Master Architectural Review: Ember Engine & ZombieFPS
**Project Scope:** Ember Forge (C++ Engine) & ZombieFPS (Lua/C++ Game)  
**Target:** AAA-Grade Wave-Based Survival Shooter

## Executive Summary
The Ember engine is a structurally sound prototype with strong intuition for Data-Oriented Design (sparse sets, packed dense arrays), pass-graph rendering (GBuffer, Forward, Post-FX), and Sol2 integration. However, the architecture currently contains critical "hard ceilings." 

The highest-leverage architectural work is not adding more renderer effects; it is hardening the runtime substrate. If you do not fix the ECS view iteration, the immediate-mode rendering, and the raw Lua pointers, the engine will inevitably bottleneck at 30–40 visible enemies. Furthermore, ZombieFPS is currently an FPS sandbox, lacking the native C++ gameplay primitives required to actually build the survival loop.

---

# PHASE 1: The Ember Engine & Editor (C++ Core)

## CRITICAL ARCHITECTURAL FLAWS

### 1. The ECS is a Sparse Set, but Iteration is NOT Data-Oriented
While `ComponentMemoryArray<T>` stores each component type densely, your iteration loops break Data-Oriented Design (DOD) entirely. 
* **The Flaw:** `View::Iterator::FindValidEntities` iterates entity IDs from one driver component and performs component-mask checks plus sparse lookups for every joined component. Hot systems then repeatedly call `Registry::GetComponent` or `Registry::GetComponents` inside the loop.
* **The Impact:** You iterate the driver's dense array, but every other component is fetched through a separate sparse→dense indirection. For a wave shooter spawning 50+ zombies touching `Transform`, `SkinnedMesh`, `RigidBody`, and `AI` components, this causes massive CPU cache-misses and pointer-chasing. 
* **The Fix:** You must use multi-component views/groups that return contiguous component references directly during iteration, bypassing the sparse-set lookup entirely for hot paths.

### 2. Destructive Registry Modification During Iteration
* **The Flaw:** In systems like `AudioSystem::UpdateAudioSources`, you check if a sound is finished, and if so, execute `scene->RemoveEntity(entity);` (or similar destructive calls) while actively iterating over the `ActiveQuery` view.
* **The Impact:** Modifying the underlying data structure (deleting entities, popping memory) while iterating causes iterator invalidation. This leads to silent skipping of entities or immediate segfaults.
* **The Fix:** Make component destruction deterministic. Implement a deferred command buffer or tag entities with a `PendingDestroyComponent`. Run a `LifecycleSystem` at the very end of the frame to safely scrub memory.

### 3. C++/Lua Boundary Safety (The Memory Time-Bomb)
Your use of Sol2 is leaking raw C++ component/entity pointers into a Garbage Collected (GC) VM.
* **The Flaw:** If Lua holds a reference to a C++ component, and C++ destroys that entity mid-frame, Lua is left with a dangling pointer. Furthermore, if C++ holds a `sol::table` (ScriptComponent) and that table holds a C++ `Entity` reference, you create a circular GC dependency. Neither language can free the memory.
* **The Fix:** Lua must **never** hold an owning reference or standard C++ pointer to an Entity. Pass lightweight, 32-bit `EntityID` handles (or weak handles with generation counters) to Lua. When Lua calls an engine API, it passes the ID back to C++ to resolve it.

### 4. Immediate-Mode Rendering & State Spam
The renderer is treating entities like immediate-mode UI rather than a batched scene.
* **The Flaw:** The engine submits draw calls immediately as it iterates entities (`Renderer2D::DrawQuad`). There is no sorting by material, texture, or depth. 
* **The Impact:** Severe OpenGL state-change overhead. You are paying for program binds and texture binds per-entity.
* **The Fix:** Convert to a **Render Item Queue**. ECS systems should generate lightweight RenderItems and push them to a buffer. Sort the buffer (Opaque front-to-back, Transparent back-to-front, grouped by Material/Shader), and submit batched arrays to the GPU.

---

## OPTIMIZATIONS & HARDENING

### 1. Asset Pipeline (Synchronous Blocking)
* **The Flaw:** `AssetManager::LoadDefaults` is fully synchronous, and `m_Assets.at(uuid)` throws on a cache miss. 
* **The Impact:** Loading a new wave of zombies, a new weapon, or streaming audio will freeze the main thread.
* **The Fix:** * Add an async load queue (worker pool that decodes textures/meshes off-thread, GPU upload on the main thread).
  * Reference-count assets so unused ones can be evicted between waves.
  * Implement a `GetAsset<T>` API that returns `SharedPtr<T>{}` on a miss (substituting a default checkerboard texture or fallback shader), preventing hard crashes.

### 2. String Hashing in the Inner Render Loop
* **The Flaw:** In `Shader.cpp`, `GetUniformLocation` uses a `std::unordered_map<std::string, int>` as a cache. Your renderer uses `SetUniform("Albedo", value)`.
* **The Fix:** Passing a `std::string` by value forces string hashing per uniform, per material, per mesh, every frame. Cache uniform locations internally within the `Material` class using integer handles, or use compile-time hashed StringIDs (e.g., `constexpr uint32_t`).

### 3. Material UBOs vs. Uniform Push
* **The Flaw:** While `CameraData` utilizes `std140` UBOs correctly, standard materials still rely on individual `uniform vec3 u_Albedo;` calls.
* **The Fix:** Group per-material properties into a Material UBO or SSBO. Bind a single buffer per material rather than 5-10 `glUniform` API calls per object.

### 4. Pathfinding Memory Allocations (A*)
* **The Flaw:** In `AStar::AStarPath`, you allocate a `std::vector<std::vector<NavNodeScratch>>` on the stack/heap every time an AI requests a path.
* **The Fix:** Pre-allocate a global scratchpad grid for the pathfinding system at level load. Reset flags via a dirty-flag frame counter rather than allocating/deallocating memory in the AI loop.

---

## ENGINE ROADMAP (Missing Features Required for ZombieFPS)

1. **GPU Instanced Skinned Meshes:** You cannot draw 50 zombies individually with CPU-calculated bone matrices. Upgrade `StandardGeometrySkinned.glsl` to support Hardware Instancing (`glDrawElementsInstanced`). Use compute shaders or pass bone matrix textures to the GPU.
2. **Recast/Detour NavMesh:** Throw away the 2D Voxel Grid (`NavigationGrid.cpp`). A commercial shooter requires verticality (stairs, ramps, overhangs). You must integrate a polygon-based 3D NavMesh.
3. **Layered Animation Blending (Masks):** Zombies and players must blend animations (e.g., legs sprinting while arms are reloading/shooting). You need a system that accepts Animation Masks to interpolate bone transforms across separate tracks.
4. **Portal/PVS Occlusion Culling:** A zombie map has dense interiors. Frustum culling (`Frustum::IsBoxVisible`) will still send every hidden zombie behind a wall to the GPU. Implement a Potentially Visible Set (PVS) or Portal system.
5. **Production Audio System:** Miniaudio needs an architectural wrapper supporting: Voice budgeting, buses/mix snapshots, attenuation curves, occlusion, reverb zones, and event-style sound pooling.

---

# PHASE 2: Game Implementation (ZombieFPS)

## CRITICAL ARCHITECTURAL FLAWS

### 1. String-Based Singleton Wiring is the Entire Architecture
Every subsystem finds its dependencies via hardcoded global string lookups.
* **The Flaw:** `WeaponFire:Fire` executes **four** `Scene.GetEntityByName("...")` lookups per shot (`WeaponAiming`, `MuzzleFlash`, `BarrelTip`, `WeaponRecoil`). `WeaponHolder:OnShoot` looks up the weapon entity by name.
* **The Impact:** This architecture is a house of cards. If you rename an entity in the editor, or try to implement split-screen/multiplayer (where two players have a `WeaponFire` entity), the entire game breaks. It is also unnecessarily slow.
* **The Fix:** Implement **Prefab-Local Child References**. Scripts must expose Entity Handle variables to the Editor UI (e.g., `self.MuzzleFlashEntity`), which are assigned at design time and resolved natively.

### 2. Missing Gameplay Primitives (The "Zombies" part doesn't exist)
ZombieFPS is currently an FPS sandbox (move, aim, fire, reload). The entire survival loop is missing from the C++ core.
* **The Fix:** Before writing zombie AI in Lua, build native C++ systems for:
  * `HealthComponent` and `DamageableComponent`.
  * A formal `DamageEvent` system (allowing for headshot multipliers, critical hits, armor).
  * A `WaveDirector` C++ system to manage the AI budget, spawn caps, and round state.
  * Make `WeaponFire.Damage` actually dispatch damage events rather than relying on script-side health math.

### 3. Hacky Raycast Origins (The Cover Problem)
* **The Flaw:** `WeaponFire` casts its gameplay ray from the `BarrelTip` to calculate hits.
* **The Impact:** In a true FPS, the visual barrel is offset to the bottom right of the screen. If you fire from the muzzle, your raycast will hit waist-high cover in front of you, even if your crosshair is aimed directly at a zombie's head over the cover.
* **The Fix:** Cast the gameplay raycast from the **Camera Center** to determine the hit entity. Then, draw the *visual* Tracer from the `BarrelTip` to the calculated hit point.

---

## OPTIMIZATIONS & REFACTORING

### 1. Formal State Machines for Weapons
* **The Flaw:** Weapon scripts are held together by `bool` flags (`IsReloading`, `IsAiming`, `IsEmpty`). 
* **The Fix:** Implement a formal Hierarchical Finite State Machine (HFSM). Weapons should mutually exclusively transition between `Idle`, `Fire`, `Reload`, `Swap`, and `Sprint` states to avoid logic spiderwebs.

### 2. Batched Async Raycasts (AI Scalability)
* **The Flaw:** (Forecasted) 50 zombies will route line-of-sight checks through `Physics.CastRay` per frame, choking the physics thread.
* **The Fix:** ReactPhysics3D supports batched queries. Expose a `Physics.CastRayBatch(jobs)` API to Lua that queues raycasts asynchronously and returns the results on the next frame.

### 3. Deterministic RNG for Recoil & AI
* **The Flaw:** There is no seed control for `Math.RandomFloat`.
* **The Fix:** If you ever want replay systems, multiplayer, or consistent recoil spray patterns, you need a Deterministic RNG service with per-stream seeding (e.g., `Random.NewStream("recoil", seed)`).

### 4. Serialization Service
* **The Flaw:** (Forecasted) There is no save state architecture.
* **The Fix:** Wave shooters require round, score, and loadout persistence. Build a serialization service using explicit `Serialize`/`Deserialize` Lua hooks per script, backed by a binary schema (since rapidyaml is already in your vendor folder).

### 5. Inventory Abstraction
* **The Flaw:** The inventory relies on hardcoded slots.
* **The Fix:** Replace hardcoded weapon slots with a data-driven inventory array that scales naturally to perks, grenades, and buildable items.