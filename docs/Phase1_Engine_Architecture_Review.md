# Phase 1: The Ember Engine & Editor

Ember Forge is a strong prototype/editor architecture, but it is not yet a data-oriented commercial shooter runtime. The dominant pattern is that the engine has ECS-shaped storage, but many hot paths still behave like object-oriented immediate mode systems: fetch entity, resolve assets, bind state, draw, repeat.

## CRITICAL Architectural Flaws

### 1. ECS queries are not truly data-oriented

`ComponentMemoryArray<T>` stores each component type densely, which is a good foundation, but `View::Iterator::FindValidEntities` iterates entity IDs from one driver component and performs component-mask checks plus sparse lookups for every joined component. Hot systems then repeatedly call `Registry::GetComponent` or `Registry::GetComponents` inside the loop.

Primary references:

- `ComponentMemoryArray<T>` in `Ember/src/Ember/ECS/Component/ComponentManager.h`
- `ComponentManager::GetActiveEntities<T>` in `Ember/src/Ember/ECS/Component/ComponentManager.h`
- `View::Iterator::FindValidEntities` in `Ember/src/Ember/ECS/View.h`
- `Registry::GetComponents` in `Ember/src/Ember/ECS/Registry.h`

For a shooter with dozens of zombies, projectiles, pickups, UI entities, audio emitters, and AI agents, this becomes pointer-chasing across component arrays instead of linear streaming over packed transform, velocity, AI, and render data. The current design is acceptable for editor convenience and small scenes, but it is not a hot-loop ECS design.

### 2. Lua component access can dangle

`ScriptBindEntity::GetComponentFromString` returns raw pointers to component storage with `sol::make_object(state, &entity.GetComponent<T>())`. Those pointers refer directly into `std::vector<T>` storage owned by `ComponentMemoryArray<T>`.

Any of the following can invalidate Lua-held component references:

- `DenseComponentArray` reallocation when attaching more components of that type.
- Swap-and-pop movement in `ComponentMemoryArray<T>::RemoveComponent`.
- Entity destruction through `Registry::DestroyEntity`.
- Scene transitions or runtime stop/reset.

Primary references:

- `ScriptBindEntity::GetComponentFromString` in `Ember/src/Ember/Script/Bindings/ScriptBindEntity.cpp`
- `ComponentMemoryArray<T>::AttachComponent` in `Ember/src/Ember/ECS/Component/ComponentManager.h`
- `ComponentMemoryArray<T>::RemoveComponent` in `Ember/src/Ember/ECS/Component/ComponentManager.h`
- `ScriptComponent` in `Ember/src/Ember/ECS/Component/Components.h`

This is the most dangerous C++/Lua lifetime flaw in the current architecture. A script can cache `transform = entity:GetComponent("TransformComponent")`, then C++ can move or destroy that component, leaving Lua with a stale native pointer.

### 3. Entity destruction bypasses normal component-detach lifecycle callbacks

`Registry::DestroyEntity` calls `m_ComponentManager->EntityDestroyed(entity)` directly, which removes component storage without firing `Registry::OnComponentDetached<T>` callbacks. Physics is partially special-cased in `Scene::RemoveEntityFromScene`, but this creates a fragile lifecycle split: some resources are manually cleaned, while others depend on detach hooks that do not run on destruction.

Primary references:

- `Registry::DestroyEntity` in `Ember/src/Ember/ECS/Registry.cpp`
- `ComponentMemoryArray<T>::EntityDestroyed` in `Ember/src/Ember/ECS/Component/ComponentManager.h`
- `Scene::RemoveEntityFromScene` in `Ember/src/Ember/Scene/Scene.cpp`
- `PhysicsSystem::OnSceneAttach` cleanup hook registration in `Ember/src/Ember/ECS/System/PhysicsSystem.cpp`

This is a systemic lifecycle bug, not a cleanup nit. Any new native resource added to a component can leak or survive past entity death unless every destruction path remembers to special-case it.

### 4. The 3D renderer is immediate submission, not batched rendering

`RenderQueueBuckets` only separates opaque, forward, and transparent entities. It does not sort by shader, material, mesh, pipeline state, texture set, or instancing compatibility. `DeferredGeometryRenderPass::Execute`, `ForwardEntitiesRenderPass::Execute`, and `TransparentEntitiesRenderPass::Execute` bind shaders/materials and submit one draw per entity. `Renderer3D::Submit` calls `material->Bind()` and immediately draws.

Primary references:

- `RenderSystem::SortEntitiesByRenderQueue` in `Ember/src/Ember/ECS/System/RenderSystem.cpp`
- `RenderQueueBuckets` in `Ember/src/Ember/Render/RenderQueueBuckets.h`
- `DeferredGeometryRenderPass::Execute` in `Ember/src/Ember/Render/Pass/DeferredGeometryRenderPass.cpp`
- `ForwardEntitiesRenderPass::Execute` in `Ember/src/Ember/Render/Pass/ForwardEntitiesRenderPass.cpp`
- `TransparentEntitiesRenderPass::Execute` in `Ember/src/Ember/Render/Pass/TransparentEntitiesRenderPass.cpp`
- `Renderer3D::Submit` in `Ember/src/Ember/Render/Renderer3D.cpp`

This will not scale to shooter scenes with repeated zombie meshes, weapons, props, decals, muzzle flashes, particles, and UI overlays. The renderer needs render-item generation plus sort keys, not per-entity material binding.

### 5. Animation is a major CPU bottleneck

`AnimationSystem::OnUpdate` allocates `localTransforms` and `globalTransforms` per animated entity per frame. `GetTrack` linearly searches tracks for every bone. `GetKeyframeIndex` linearly scans keyframes.

Primary references:

- `AnimationSystem::OnUpdate` in `Ember/src/Ember/ECS/System/AnimationSystem.cpp`
- `GetTrack` in `Ember/src/Ember/ECS/System/AnimationSystem.cpp`
- `GetKeyframeIndex` in `Ember/src/Ember/ECS/System/AnimationSystem.cpp`
- `AnimatorComponent` in `Ember/src/Ember/ECS/Component/Components.h`

For a zombie shooter, many enemies will share skeletons and clips. Current evaluation cost scales badly with animated entity count and bone count. This needs cached bone-to-track maps, persistent pose buffers, clip sampling caches, animation LOD, and likely job scheduling.

### 6. Pooling teardown can leave runtime entities behind

`Pool::Clear` iterates using `i < m_AvailableEntities.size()` while popping from the queue, so it can skip entries as the size shrinks. It also clears only available entities, not active ones. `PoolManager::DestroyPools` therefore does not guarantee pooled entities are actually removed.

Primary references:

- `Pool::Clear` in `Ember/src/Ember/Core/Pool.cpp`
- `Pool::Retrieve` in `Ember/src/Ember/Core/Pool.cpp`
- `Pool::Return` in `Ember/src/Ember/Core/Pool.cpp`
- `PoolManager::DestroyPools` in `Ember/src/Ember/Core/PoolManager.cpp`

For bullets, pickups, zombie attacks, temporary audio emitters, and VFX, this creates stale runtime state across play sessions and scene transitions.

### 7. Script VM ownership is unfinished

`ScriptEngine::Init` allocates `s_LuaState` with `new`, but `Application::~Application` has `ScriptEngine::Shutdown()` commented out. Runtime stop does correctly clear `sol::table` instances before replacing the Lua VM, but the global VM lifecycle is still leaky and depends on shutdown ordering discipline.

Primary references:

- `ScriptEngine::Init` in `Ember/src/Ember/Script/ScriptEngine.cpp`
- `ScriptEngine::Shutdown` in `Ember/src/Ember/Script/ScriptEngine.cpp`
- `ScriptEngine::OnRuntimeStop` in `Ember/src/Ember/Script/ScriptEngine.cpp`
- `Application::~Application` in `Ember/src/Ember/Core/Application.cpp`

The engine needs one authoritative Lua VM ownership model, with deterministic teardown before any `sol::table`, `sol::function`, or Lua-bound component userdata can outlive its state.

## OPTIMIZATIONS

### Introduce grouped ECS views for hot joins

For common joins like `TransformComponent + RigidBodyComponent`, `TransformComponent + StaticMeshComponent + MaterialComponent`, and `TransformComponent + AnimatorComponent`, build grouped storage or cached query results. The current driver-plus-mask query is fine for editor tooling, but not for transform, render, physics, animation, AI, or audio hot paths.

### Split large/runtime-heavy component payloads

Several components are too heavy for cache-friendly iteration:

- `AnimatorComponent` owns two `std::vector<Matrix4f>` caches.
- Collider components own RP3D pointers and mesh arrays.
- `NavigationGridComponent` owns `std::vector<std::vector<NavNode>>`.
- `ScriptComponent` owns `sol::table` plus an override map.

Move heavy runtime allocations into system-owned pools keyed by `EntityID` or stable handles. Leave components as small indices, handles, flags, or compact POD state.

### Cache script classes and functions

`ScriptSystem::InitializeScriptForEntity` runs `luaState.script_file(filepath)` per entity instance, and `ScriptSystem::OnUpdate` performs `script.Instance["OnUpdate"]` lookup every frame.

Primary references:

- `ScriptSystem::InitializeScriptForEntity` in `Ember/src/Ember/ECS/System/ScriptSystem.cpp`
- `ScriptSystem::OnUpdate` in `Ember/src/Ember/ECS/System/ScriptSystem.cpp`
- `ScriptEngine::GetScriptProperties` in `Ember/src/Ember/Script/ScriptEngine.cpp`

Cache script class tables per `Script` asset and cache protected functions per instance. Also avoid reparsing script files for property discovery when the asset has already been loaded and inspected.

### Build render commands once per frame, then sort

`RenderSystem::StoreRenderableEntities` rebuilds renderable pairs and AABBs every frame, then `RenderSystem::SortEntitiesByRenderQueue` only buckets by render queue.

Replace this with a render item stage:

```cpp
RenderItem {
    EntityID Entity;
    MeshHandle Mesh;
    MaterialHandle Material;
    ShaderHandle Shader;
    Matrix4f Transform;
    SkinningHandle Skinning;
    AABB Bounds;
    uint64_t SortKey;
}
```

Opaque sort key should group by pipeline/shader/material/mesh. Transparent sort key should include depth. This is the bridge from an editor renderer to a runtime renderer.

### Add a GL state cache

`RenderAction` maps directly to OpenGL calls, and `RendererAPI` repeatedly calls `glEnable`, `glDisable`, `glBindTextureUnit`, `glUseProgram`, and `glBindFramebuffer` without state filtering.

Primary references:

- `RenderAction` in `Ember/src/Ember/Render/RenderAction.h`
- `OpenGL::RendererAPI` in `Ember/src/Ember/Platform/OpenGL/RendererAPI.cpp`
- `OpenGL::Shader::Bind` in `Ember/src/Ember/Platform/OpenGL/Shader.cpp`

A small state cache would immediately reduce redundant driver traffic and make render pass state transitions explicit.

### Replace per-path AI allocations

`AStar::AStarPath` allocates a full scratch grid every path calculation. `AISystem::OnUpdate` can run this per dynamic agent on interval.

Primary references:

- `AStar::AStarPath` in `Ember/src/Ember/AI/AStar.cpp`
- `AISystem::OnUpdate` in `Ember/src/Ember/ECS/System/AISystem.cpp`
- `NavigationGridComponent` in `Ember/src/Ember/ECS/Component/Components.h`

Use reusable scratch buffers, flat arrays instead of `std::vector<std::vector<...>>`, and path job scheduling. Dynamic pathing should not allocate a new grid-sized scratch structure per agent.

## FEATURE SUGGESTIONS

### 1. Production animation graph

Required features:

- Blend trees.
- Masks and animation layers.
- Additive animation.
- Root motion.
- IK for feet, aim, hands, and weapon alignment.
- Animation notifies/events.
- Animation LOD.
- Shared clip sampling for enemies using the same skeleton and clip.

Current animation support is good enough for basic playback and crossfade, but a commercial shooter needs layered upper/lower body behavior, hit reactions, aim offsets, reload interactions, and enemy locomotion variation.

### 2. Real navigation stack

Required features:

- Generated navmesh, not only grid A*.
- Dynamic obstacles.
- Off-mesh links.
- Path corridors.
- Crowd/local avoidance.
- Async pathfinding.
- Runtime nav debug tooling.

The current grid A* is useful for prototypes, but zombie AI in a commercial 3D shooter needs robust movement through doors, stairs, chokepoints, vaults, spawn zones, destructible blockers, and dynamic player-created congestion.

### 3. Renderer scalability layer

Required features:

- Material/mesh sort keys.
- Hardware instancing.
- LODs.
- Occlusion or visibility culling.
- Decals.
- GPU particles.
- Render graph or frame graph instead of a fixed hand-wired pass chain.

The current deferred-plus-forward pipeline is a functional base, but the submission model is still too immediate-mode for repeated enemies, props, pickups, and combat effects.

### 4. Asset streaming and async jobs

Required features:

- Background mesh, texture, animation, and audio loading.
- Shader warmup and pipeline preparation.
- Resource residency budgets.
- Safe handle invalidation.
- Job system for animation, culling, AI, and streaming.

Current asset access is mostly synchronous and pointer-centric. A commercial shooter needs predictable frame times under load, not blocking asset fetches or asset construction in gameplay paths.

### 5. Production audio system

Required features:

- Voice budgeting.
- Buses and mix snapshots.
- Attenuation curves.
- Occlusion and obstruction.
- Reverb zones.
- Streaming audio.
- Event-style sound playback with pooling.

The current miniaudio integration has basic playback and spatial hooks, but not a shippable shooter audio pipeline. Zombie hordes, weapons, impacts, announcer lines, ambience, and UI all need prioritization and mixing rules.

## Bottom Line

The engine is in a promising editor/prototype phase. The highest-leverage architectural work is not adding more renderer effects or more components; it is hardening the runtime substrate:

1. Make ECS hot paths truly contiguous and grouped.
2. Replace raw Lua component pointers with safe handles or short-lived accessors.
3. Make component destruction lifecycle deterministic and universal.
4. Convert renderer submission from entity-immediate to sorted render items.
5. Move animation, pathfinding, and culling toward persistent buffers and jobs.

Until those are addressed, Ember Forge will keep hitting scaling cliffs exactly where a wave-based shooter applies pressure: many animated enemies, many short-lived objects, lots of script interaction, and repeated renderable assets.