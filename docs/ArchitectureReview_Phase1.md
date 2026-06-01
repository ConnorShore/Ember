# Phase 1 — Architectural Review: Ember Engine & Editor

The codebase is well-organized for a one-person engine. There is real DOD intuition (sparse sets, packed dense arrays, batched 2D renderer, GBuffer) and the pass graph is reasonable. But there are **several systemic flaws that will become hard ceilings the moment you push past a small demo scene**, and the renderer in particular cannot ship a wave shooter at AAA quality in its current shape.

File references are 1-based.

---

## CRITICAL ARCHITECTURAL FLAWS

### C1. The "ECS" is a sparse set, but iteration is *not* data-oriented
`Ember/src/Ember/ECS/View.h`, `Ember/src/Ember/ECS/Registry.h`

`View::Iterator::FindValidEntities` and `GetComponents<...>` produce iteration that looks like:

```
for (EntityID e : DriverDenseEntityArray)
    if (Registry::ContainsComponents<F1,F2,...>(e)) // sparse lookup per filter
        auto [a,b,c] = Registry::GetComponents<A,B,C>(e); // sparse → dense per type
```

You iterate the driver's dense entity array, but every other component is fetched through a *separate* sparse→dense indirection. That defeats the entire reason to have a packed `DenseComponentArray<T>`: you never walk components in linear memory order for any non-driver type. For a wave shooter spawning 50–200 zombies that all touch `TransformComponent`, `SkinnedMeshComponent`, `RigidBodyComponent`, `AnimatorComponent`, `MaterialComponent`, every system pays N × (k filter checks + k indirections + k bound checks) per frame.

Worse — `Registry::ContainsComponent<T>` resolves through `EntityManager::ContainsComponent` which does a `bitset<MaxEntities>::test` on a per-entity bitset (1 cache line per entity), but `GetComponent<T>` then calls `ComponentManager::GetComponent<T>` which does **`StaticPointerCast<ComponentMemoryArray<T>>(m_ComponentArrays[type])`** — see C2.

This is workable for prototyping; it is *not* what your sparse-set storage promises. Your `View` design needs a "group" / "owning group" concept (entt-style) where you reorder a small number of dense arrays so iteration over multiple component types is genuinely linear.

### C2. `SharedPtr` overhead is on the hot path of *every* ECS access
`Ember/src/Ember/Core/SharedPointer.h`, `Ember/src/Ember/ECS/Component/ComponentManager.h`

Component arrays are stored as `std::vector<SharedPtr<ComponentMemoryArraysBase>>`. Every `GetComponent<T>`, `AttachComponent<T>`, `ContainsComponent<T>`, and `GetActiveEntities<T>` performs:

```cpp
SharedPtr<ComponentMemoryArray<T>> memoryArrays = StaticPointerCast<...>(m_ComponentArrays[type]);
```

`StaticPointerCast` constructs a *new* `SharedPtr<T>` from a raw pointer, which calls `IncrementRefCount()` and the temp's destructor calls `DecrementRefCount()` — **two atomic RMW operations per ECS access**. Multiplied by the View pattern in C1, you have tens of thousands of redundant atomic ops per frame.

Component arrays have a single owner (the `ComponentManager`). Use `std::unique_ptr` or a raw `T*` cache. There is no sharing semantics here — `SharedPtr` is purely a tax.

### C3. `SharedPtr(T*)` will null-dereference; `StaticPointerCast` is unsafe on null inputs
`Ember/src/Ember/Core/SharedPointer.h`

```cpp
SharedPtr(T* ptr) : m_Ptr(ptr) { m_Ptr->IncrementRefCount(); }   // no null check
template <typename T, typename U>
SharedPtr<T> StaticPointerCast(const SharedPtr<U>& ptr)
{ return SharedPtr<T>(static_cast<T*>(ptr.Ptr())); }              // forwards null
```

If `ptr.Ptr()` is null (e.g., `RenderSystem::GetRenderPass("Typo")`, `AssetManager::GetAssetBase` on a missing asset, any cast of an empty handle), this crashes inside the constructor. Either reject null in the raw ctor, or have `StaticPointerCast` short-circuit. The `Application::Instance().GetAssetManager().GetAsset<...>(...)` calls in every render pass currently rely on the asset always existing — that's a release-mode crash waiting on the first missing asset.

### C4. Lua bindings hand out raw C++ component pointers — *all of them are dangling references waiting to happen*
`Ember/src/Ember/Script/Bindings/ScriptBindEntity.cpp`, `Ember/src/Ember/ECS/Component/ComponentManager.h`

`GetComponentFromString` returns `sol::make_object(state, &entity.GetComponent<T>())` — a raw pointer into `DenseComponentArray<T>`. That pointer is stable **only until the next `AttachComponent<T>`** anywhere in the engine, which `emplace_back`'s into the same `std::vector<T>` and reallocates, *or* **until the next `DetachComponent<T>`**, which does swap-and-pop and **moves a different component into that slot**.

Concretely: Lua holds `self.transform = entity:GetComponent("TransformComponent")`. Anywhere later in the same frame, anything spawning a new entity with a TransformComponent can reallocate the dense vector, and `self.transform` is now a dangling pointer pointing into freed storage (or worse, into another entity's transform after a swap-and-pop). For a wave shooter that spawns/destroys enemies constantly, this is a silent memory-corruption cannon.

Fixes (any one):
- Reserve a hard cap up front per component vector and assert.
- Return a thin handle `{EntityID, ComponentType}` to Lua and re-resolve through the registry on every access (this is what Unity does).
- Use stable storage (deque-like / chunked pool) for component arrays.

### C5. Per-frame draw submission has no batching, no instancing, and no state sorting
`Ember/src/Ember/Render/Renderer3D.cpp`, `Ember/src/Ember/Render/Pass/DeferredGeometryRenderPass.cpp`, `Ember/src/Ember/Render/Pass/ForwardEntitiesRenderPass.cpp`

`Renderer3D::Submit` is literally:
```cpp
material->Bind();
material->GetShader()->SetMatrix4("u_Transform", transform);
RenderAction::DrawIndexed(vertexArray);
```
And the geometry pass loops the opaque bucket calling `materialAsset->GetShader()->Bind()` *per entity*. There is:
- **No state sort** — opaque bucket is not sorted by shader→material→mesh, so `glUseProgram` and texture rebinds happen redundantly across consecutive draws.
- **No instancing** — 50 zombies sharing the same skinned mesh = 50 draw calls + 50 `SetMatrix4Array("u_BoneMatrices", 100 matrices)` UPLOADS in the geometry pass alone, plus another 50 in the shadow pass and another 50 in any forward pass. With cascaded shadows or lights this gets multiplied again.
- **No redundant-state caching** in `RenderAction` (no last-bound shader / VAO / texture comparison).
- **Per-entity asset map lookup** — `Application::Instance().GetAssetManager().GetAsset<MaterialBase>(handle)` and `GetAsset<Mesh>(handle)` are `unordered_map` lookups + `SharedPtr` ref-count traffic, every entity, every pass.

For a Zombies-style game your bottleneck won't be GPU shading — it will be CPU-side draw call submission. You need:
1. Sort the opaque bucket by `(shaderID, materialID, meshID)` once per frame.
2. Merge consecutive draws sharing mesh+material into `glDrawElementsInstanced` with per-instance transforms in an SSBO.
3. Cache `Mesh`/`MaterialBase` pointers when you build `RenderQueueBuckets` so the inner pass doesn't re-resolve handles.

### C6. Bone matrices uploaded as a giant uniform array per skinned draw
`Ember/src/Ember/Render/Pass/DeferredGeometryRenderPass.cpp`, `Ember/src/Ember/Render/Pass/ForwardEntitiesRenderPass.cpp`

`SetMatrix4Array(Constants::Uniforms::BoneMatrices, ..., MaxBones)` per skinned entity per pass. Plus the static identity-fallback path uploads `MaxBones` (100+) identity matrices for any skinned mesh that hasn't yet linked an animator. That is `MaxBones * 64 bytes ≈ 6–25 KB` of uniform data per draw, per pass, per entity. Move bone palettes into a single SSBO updated once per skinned entity per frame and indexed by an instance ID; this also unblocks instanced skinned rendering.

### C7. Render pass / post-process pass lookups are string-keyed every frame
`Ember/src/Ember/ECS/System/RenderSystem.cpp`

```cpp
auto deferredGeometryPass = StaticPointerCast<DeferredGeometryRenderPass>(GetRenderPass("DeferredGeometryRenderPass"));
```
Every frame, ~14 `unordered_map<string,...>` lookups + 14 atomic SharedPtr operations — for fixed, never-changing pass identities. Cache typed members at `OnAttach` time (`m_DeferredGeometryPass = ...`) and call directly.

### C8. `EntityManager` allocates O(MaxEntities) of fixed-cost storage regardless of usage
`Ember/src/Ember/ECS/Entity/EntityManager.h`

```cpp
std::bitset<MaxComponents>            m_AliveEntities;            // ok
std::array<std::bitset<MaxComponents>, MaxEntities> m_EntityComponentMask;
std::array<std::vector<ComponentType>, MaxEntities> m_EntityComponentOrder;
```
With a typical `MaxEntities` in the tens of thousands, this is hundreds of KB to MBs of cold memory permanently resident even for an empty editor scene, and it makes scene/registry copies expensive. The `m_EntityComponentOrder` `std::vector` per slot is purely an editor concern (it tracks UI display order) — it should not live in the runtime entity manager at all. Move it to an editor-side map keyed only on entities that the user has explicitly reordered.

Also: `ComponentMemoryArray<T>::SparseEntityArray.resize(MaxEntities, ...)` happens at *construction*, so every component type ever registered allocates an N-sized sparse array up front, even for component types that only attach to one entity (e.g. `CameraComponent`, `DirectionalLightComponent`).

### C9. Throwing `std::runtime_error` from per-frame Lua callbacks tears the engine down
`Ember/src/Ember/ECS/System/ScriptSystem.cpp`

A scripting error in any entity's `OnUpdate`, trigger event, or animation event throws out of the system update loop. In a shipped game, one bad enemy script equals an instant crash. Standard practice: log + disable that script's `Initialized` flag (or set `script.Errored`) and continue. Optionally allow re-enabling after hot-reload.

### C10. `TransformSystem` walks the hierarchy via UUID hash lookups every frame
`Ember/src/Ember/ECS/System/TransformSystem.cpp`

```cpp
for (UUID child : relationship.Children)
    UpdateTransformTree(scene->GetEntity(child).GetEntityHandle(), ...);
```
`scene->GetEntity(uuid)` is an `unordered_map<UUID, EntityID>` lookup performed for every child node every frame. For deep prefab hierarchies (weapons → magazines → bullets → muzzle flashes) this dominates the system update. Store children by `EntityID` (the registry-stable handle) instead of `UUID`; UUIDs are an asset-layer concept, not a runtime pointer. The same anti-pattern lives in `ScreenSpace2DRenderPass::FindNearestCanvasAncestor` and `PhysicsSystem::FindRigidBodyEntity` — UUID walking is wired into multiple per-frame hot paths.

---

## OPTIMIZATIONS (order by impact)

1. **State-sort + instance the opaque bucket.** Build a sort key `(shaderId<<48 | materialId<<24 | meshId)` when you populate `RenderQueueBuckets::Opaque`, sort once, then collapse consecutive identical `(shader,material,mesh)` runs into one instanced draw with an SSBO of transforms. Single biggest CPU win you have available.
2. **Cache resolved asset pointers in the bucket sort.** Replace `RenderQueueBuckets::Opaque = vector<EntityID>` with `vector<RenderItem{ EntityID, Mesh*, Material*, mat4 }>` so passes don't redo `GetAsset` lookups. Also fixes the redundant work between deferred-geometry and shadow passes that re-resolve the same handles.
3. **Drop `SharedPtr` from the ECS storage layer** (`ComponentManager.h`). Use `std::unique_ptr<ComponentMemoryArraysBase>`. Removes thousands of atomic ops per frame.
4. **Pre-compute the opaque/forward/transparent buckets once and reuse.** `StoreRenderableEntities` already does culling; share its results with the shadow pass instead of re-iterating the registry per pass.
5. **Cache pass pointers in `RenderSystem`** as typed members. Eliminate 28 `unordered_map<string>` lookups per frame.
6. **Replace `m_EntityComponentOrder` array-per-entity** with an editor-side `unordered_map<EntityID, vector<ComponentType>>` and only populate it for reordered entities.
7. **Add a bone-id → track-index cache** on `Animation` (the TODO is already noted in `AnimationSystem::GetTrack`); current linear search per bone per frame per skinned entity is O(bones × tracks).
8. **Cache `RectTransform`'s nearest `CanvasComponent` ancestor** once on attach (and invalidate on reparent) instead of walking the parent chain every frame in `ScreenSpace2DRenderPass`.
9. **Fix `ComponentManager::EntityDestroyed`** — `for (auto compArray : m_ComponentArrays)` copies a `SharedPtr` per array per destroyed entity. Use `auto&`.
10. **Renderer2D batch is hardcoded to `MaxQuads = 1024`**. Any HUD with more than 1024 quads (text-heavy ammo counters, minimap markers) silently triggers `NextBatch()`, which flushes mid-frame. Either increase, or expose as a config; also pre-size to typical worst-case.

---

## FEATURE SUGGESTIONS — what you must add to ship a 3D shooter

### F1. NavMesh runtime (Recast/Detour or equivalent)
Your `NavigationGrid` + A* is a uniform grid against `PhysicsSystem` raycasts. That's adequate for a top-down survival prototype; it is **not** adequate for COD: Zombies–style levels with multi-floor navigation, jump links, ramps, and dynamic boarding-up of windows. Integrate Recast for static bake + Detour for runtime queries; expose `NavMeshAgent`/`NavMeshObstacle` components. Otherwise your AI pathing breaks the moment a designer adds a staircase.

### F2. Animation layer masking + additive blending
`AnimationSystem.cpp` implements a single full-body blend between two clips. A first/third-person shooter needs:
- **Bone masks** per layer (lower body = locomotion, upper body = aim/reload/shoot).
- **Additive layers** (recoil kick, lean, breathing).
- **Foot IK** (two-bone IK solver) — without this, ramps and stairs look universally awful.
- **Aim look-at IK** for head/spine.

A shooter without these reads as last-gen the moment the player turns 90°.

### F3. Real audio spatialization & runtime DSP
`AudioSystem.cpp` calls `ma_sound_set_position` and that's it. Missing:
- **Doppler velocity** (you don't track per-source velocity; pass to `ma_sound_set_velocity`).
- **Audio buses / submix groups** (Master / SFX / Music / VO / UI) with per-bus volume + DSP.
- **Reverb zones / convolution reverb** (interior vs. exterior).
- **Occlusion / obstruction** (raycast between listener and emitter; low-pass + attenuate when occluded).
- **Voice/priority management** (cap concurrent voices, kill quietest first; mandatory once a horde fires SMGs).
- **Audio events / RTPC**-style abstraction so designers can author "one-shot vs. looping vs. interactive music".

### F4. GPU-driven culling and a proper visibility pipeline
Currently `StoreRenderableEntities` does CPU-side AABB frustum culling. For dense interior zombie scenes you also need:
- **Hi-Z occlusion culling** (compute pass against down-sampled depth pyramid).
- **Per-pass cull results reuse** (cull once, reuse for shadows + main pass with per-cascade refinement).
- **Indirect draw via `glMultiDrawElementsIndirect`** so culling output drives draws without round-tripping to the CPU.

### F5. Asynchronous streaming asset system + hot reload
`AssetManager::LoadDefaults` is fully synchronous and `m_Assets.at(uuid)` throws. To ship anything beyond a single arena:
- Add an async load queue (worker pool that decodes textures/meshes off-thread, GPU upload on the main thread).
- Reference-count assets so unused ones can be evicted between waves/levels.
- Hot-reload for shaders and scripts in editor builds only — saves enormous iteration time when balancing weapons/zombies.
- A `GetAsset<T>` API that returns `SharedPtr<T>{}` on miss (substituting fallback shader/material), never throws.

### F6 (bonus). Replace the Lua component-pointer binding with handles
Tied to C4 above. Without this, every gameplay system you write in Lua is a memory-corruption time-bomb the moment you spawn/destroy entities mid-frame. This isn't optional for a wave shooter.

---

**Bottom line:** the architecture *direction* is correct (sparse-set ECS, deferred + forward hybrid, post-FX stack, Lua scripting, ReactPhysics3D). The implementation has three systemic issues that will gate shipping: (1) the renderer submits one draw per entity with no batching/instancing/state sorting, (2) the ECS storage layer pays atomic ref-count traffic on every access *and* iteration is not actually data-oriented for non-driver components, and (3) the C++↔Lua boundary leaks raw component pointers into a GC'd VM. Fix those three and the engine is a credible base for a commercial wave shooter; skip them and you'll bottleneck at 30–40 visible enemies.
