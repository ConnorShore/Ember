# Ember Engine — TODO Triage

Prioritized list of all `// TODO:` comments across the **Ember**, **Ember-Forge**, **Ember-Tools**, and **Ember-Runtime** projects, organized to get you to "first-game-ready" as fast as possible.

The guiding question for each item: **"Does this block, frustrate, or limit me while building a real game in the editor?"**

---

## Priority Legend

| Tier | Meaning |
|------|---------|
| **P0 — Blockers** | Bugs / missing functionality that will bite you immediately when authoring a real scene. Fix before starting the game. |
| **P1 — High Impact for MVP** | Will noticeably improve productivity / playability during game development. Tackle early. |
| **P2 — Nice-to-Have for MVP** | Useful polish or workflow improvements; do them when convenient or when they start hurting. |
| **P3 — Post-MVP** | Optimizations, refactors, code-hygiene, or features only needed at scale. Defer. |

---

## P0 — Blockers (do these first)

These are correctness bugs or missing data that will visibly break workflows while building a game.

### Recently fixed (found by Ember-Test)

- ~~**Destroying a rigid body could assert on the next physics step**~~ — FIXED (vendored ReactPhysics3D). Found in ZombieFPS, not by the suite: grabbing two nuke powerups in quick succession died on `assert(mMapEntityToComponentIndex.containsKey(bodyEntity))` in `BodyComponents::getBody()`. rp3d queues "overlap lost" pairs in `CollisionDetectionSystem::mLostContactPairs` from `computeBroadPhase()`, which also runs **outside** `PhysicsWorld::update()` — every `testOverlap()`/`testCollision()` snapshot query calls it, and Ember runs several per frame (`UpdateAvoidanceCollisions` per AI agent, `GetOverlappingVolumes` for the camera sensor). The queue is only drained by the next `update()`, and each entry names its bodies by rp3d entity, so any body destroyed in that window was looked up after it stopped existing. Every unpooled `Scene::RemoveEntity` of a physics object was exposed — pickups, tracers, impact effects, dropped weapons, `LifetimeComponent` expiries. Fixed at the reporting boundary: `PhysicsWorld::isLostContactPairStillValid()` drops pairs whose bodies or colliders are gone, called from both `OverlapCallback::CallbackData` and `CollisionCallback::CallbackData`. Guarded by `Physics::DestroyingABodyWithAQueuedOverlapExitDoesNotAssert`. Note the rp3d premake defines no `NDEBUG`, so its `assert`s are live in **every** config including Dist.
- ~~**Bone-parented entities never moved with the animation**~~ — FIXED. Two independent defects that stacked into one symptom: hitboxes parented to a rig's bone entities stayed frozen at the T-pose while the character animated.
  1. `AnimationSystem` writes the evaluated pose only into `AnimatorComponent::BonePoseMatrices`/`BoneMatrices` (GPU skinning). The bone entities the model importer leaves in the scene hierarchy carry nothing but their bind-pose TRS, so `TransformSystem` rebuilt them — and every child hanging off them — at the T-pose forever. `BoneSocketSystem::UpdateBoneDrivenEntities` now pushes the animator's pose into those entities (matched to bones by name, lifted into world space by the skinned mesh entity's transform, the same basis the renderer skins in) and recomposes their non-bone descendants.
  2. `PhysicsSystem` attached child-entity colliders to the ancestor rigid body with a **fixed** local transform and only ever re-derived it in the editor path (`RebuildEditorColliders`). At runtime a collider on a child entity therefore stayed wherever it was built, whatever the entity did afterwards — bone-driven hitboxes, scripted attachments, anything. `SyncChildColliderTransforms` re-pushes the relative pose each frame when it drifts (shape dimensions are left alone; resizing means rebuilding the shape). Guarded by `Animation::BoneEntitiesFollowTheAnimatedPose`, `Animation::BoneEntityPoseIsRelativeToTheSkinnedMesh`, `Animation::NonBoneEntitiesKeepTheirHierarchyTransform`, `Physics::ChildColliderFollowsItsEntityAtRuntime`.
- ~~**Null deref on an animator with no controller**~~ — FIXED. `AnimationSystem::OnUpdate` deliberately left `controller` null when `ControllerHandle == InvalidUUID`, then dereferenced it on the very next line (`if (controller->GetLayers().empty())`). Any entity with an `AnimatorComponent` and a valid skeleton but no animation controller — a rig imported before its controller was authored — took the frame down. Now `if (!controller || controller->GetLayers().empty()) continue;`, which also makes the redundant `if (controller)` guard further down unnecessary. Guarded by `Animation::AnimatorWithoutAControllerIsSkipped`.
- ~~**Entity 0 was invisible to every physics query**~~ — FIXED. `EntityID` was stored in rp3d's per-body/per-collider `void*` user data by casting it directly, so **entity 0 encoded as `nullptr`** — indistinguishable from "no user data" (which is the case for the camera sensor and for the temporary probe bodies overlap queries create). Every reader null-checks before decoding, so entity 0 was silently dropped from `TestOverlapSphere`/`TestOverlapBox` results and reported as `InvalidEntityID` by `CastRay`'s `ColliderEntity`. Since the first entity created in a scene gets handle 0, this hit whatever the level was built around first. Now encoded biased-by-one via `EncodeEntityUserData` / `DecodeEntityUserData` in `Physics/ColliderUserData.h`; all six read/write sites go through them. Guarded by `Physics::RaycastHitsAColliderAndReportsTheEntity`, `Physics::OverlapSphereFindsOnlyNearbyBodies`, `Physics::OverlapBoxFindsColliders`.
  - Follow-on fix in `PhysicsSystem::UpdateScriptTriggers`: trigger pairs involving an ownerless body are now skipped. Previously they decoded to entity 0, so the camera sensor fired spurious `OnOverlapTrigger*` callbacks at whichever real entity happened to hold handle 0.
- ~~**Re-enabling a disabled physics object left it frozen forever**~~ — FIXED (worked around). This is a bug in the **vendored ReactPhysics3D**: `RigidBody::setIsActive()` (`src/body/RigidBody.cpp:1185`) calls `setIsSleeping(!isActive)` *before* `Body::setIsActive(isActive)`, and `setIsSleeping()` early-returns while the body is still flagged inactive. Re-activating therefore left the body active-but-sleeping, so it never integrated again. Worked around in Ember's `DisabledComponent` detach hook by calling `setIsSleeping(false)` after `setIsActive(true)`. **The proper fix belongs upstream in the fork** — swap those two lines so `Body::setIsActive` runs first. Guarded by `Physics::DisabledEntityStopsSimulating`.

- ~~**Calling `ScriptEngine::BindAPI` more than once per Lua state corrupted the `Entity` usertype**~~ — FIXED. `BindAPI` re-ran every binder on each call, including scene-independent ones like `BindEntity`. Re-registering an already-registered usertype against a live state leaves previously handed-out values unreliable: `Entity` userdata intermittently came back with no usable `__index`, so any `entity:Method()` raised `attempt to index a sol.Ember::Entity value` from Lua. Play sessions never noticed because `OnRuntimeStart` creates a **fresh** state and binds it exactly once, but `Ember-Test` binds one shared state ~26 times (once per script test), so roughly one entity-touching script test failed per run — *which* one depending only on execution order. The same test passed under `--run=GetScriptInstance` and failed under a full run, and a script using no inheritance at all was affected, which is what ruled out the `Base` feature it was originally blamed on. `BindAPI` now registers the scene-independent set once per state (`s_StatelessBindingsRegistered`, cleared in `CreateConfiguredLuaState`) and rebinds only the four scene-capturing binders — `BindScene`, `BindPhysics`, `BindAIComponents`, `BindAudio`. `BindAllComponents` was removed rather than left as a footgun that re-registers the stateless component usertypes. Guarded by `Script::GetScriptInstanceMatchesEveryLevelOfAMultiLevelBaseChain` and the other `GetScriptInstance` tests.
  - Follow-on fix in `Application`'s constructor: `ScriptEngine::Init()` ran **before** `m_SaveGameManager` was created, and `BindSaveGame` publishes `GameData` as `&Application::Instance().GetSaveGameManager()` — i.e. it dereferenced a null `ScopedPtr` and bound the resulting garbage pointer. Latent UB that had been invisible because every subsequent `BindAPI` re-ran the binder and overwrote it; binding once per state made it permanent and broke `GameData` outright. The manager is now constructed before `ScriptEngine::Init()`. Guarded by `Script::SaveFileGettersBindEveryArity`, `Script::GameDataHandlesAddressSeparateFiles`.
- ~~**Use-after-free in `Scene::DuplicateEntityRecursive`**~~ — FIXED. It captured `auto& newRels = newEntity.AttachComponent<RelationshipComponent>()` and then wrote `newRels.Children.push_back(...)` *after* recursing to duplicate each child. Every recursion calls `AddEntity`, which attaches a `RelationshipComponent` and can reallocate that component's dense `std::vector`, invalidating `newRels` — so the child UUIDs were pushed into freed memory and a duplicated parent silently lost its children. Now collects them into a local vector and re-fetches the component after the loop. Guarded by `Scene::DuplicateEntityCopiesChildren`.
- ~~**Re-parenting did not invalidate the child's world transform**~~ — FIXED. `Entity::AddChild` only rewrote `RelationshipComponent`, and `TransformSystem::UpdateTransformTree` skips any node whose `IsLocalDirty()` is false and whose parent did not change — so a freshly re-parented child kept the stale world transform it had as a root, i.e. dragging an entity onto a parent in the editor left it visually in the wrong place. Added `TransformComponent::InvalidateWorld()` and call it from `Entity::AddChild` (both overloads), `Entity::RemoveFromParent`, `Scene::SetEntityParent`, `Scene::RemoveParent` and `Scene::DuplicateEntityRecursive`. Guarded by `Scene::AttachmentChildIgnoresParentScale`.

### Bugs

- [Ember/src/Ember/Scene/Entity.cpp](Ember/src/Ember/Scene/Entity.cpp#L8) — `GetAllChildren()` may be constructing new `Entity` objects rather than fetching from the scene registry. Parenting / hierarchy traversal is core; any subtle bug here will surface as ghost entities, orphaned references, or broken prefab updates.
- [Ember-Forge/src/EditorLayer.cpp](Ember-Forge/src/EditorLayer.cpp#L858) — Transform decompose produces NaNs on negative scaling. **Guaranteed to bite you** the moment you flip a mesh in the editor or import any mirrored asset. Either fix the math or detect negative scales and route through a different path.
- [Ember/src/Ember/Asset/AnimationSerializer.cpp](Ember/src/Ember/Asset/AnimationSerializer.cpp#L29) — "Test event serialization when scene is saved." If animation events aren't round-tripping through save/load, scripted gameplay tied to animations will silently fail.
- [Ember/src/Ember/Scene/Entity.h](Ember/src/Ember/Scene/Entity.h#L66) — `Entity` declares a **non-explicit, non-const** `operator EntityID()` alongside `explicit operator bool() const`. Because the non-const conversion binds the implicit object argument better than the const one, **`static_cast<bool>(entity)` silently calls `operator EntityID()`**, not `operator bool()`. The result is inverted in both directions: a valid entity with handle `0` reads as `false`, and an invalid entity (handle `InvalidEntityID` = 1025) reads as `true`. This is almost certainly why engine code writes `entity != Constants::Entities::InvalidEntityID` everywhere instead of the natural `if (entity)`. Making `operator EntityID()` explicit (or removing it in favour of `GetEntityHandle()`) would fix the trap; until then `if (entity)` and `static_cast<bool>(entity)` are landmines. Found when 12 tests in `Ember-Test` failed symmetrically; they now use `Entity::IsValid()`.
- [Ember/src/Ember/Asset/AssetManager.h](Ember/src/Ember/Asset/AssetManager.h#L247) — `AssetManager::Register()` stores `asset->GetFilePath()` into `m_AssetPaths` **verbatim**, while `Load()`, `RenameAsset()`, `ContainsAssetWithPath()` and `GetAssetByPath()` all normalise through `std::filesystem::absolute()` first. An asset registered under a relative path is therefore invisible to every path lookup — including `RenameAsset`'s collision check, which will then let two assets claim the same file. Normalise in `Register()` too.
- [Ember/src/Ember/Render/Camera.cpp](Ember/src/Ember/Render/Camera.cpp#L40) — `Camera::SetPerspective()` / `SetOrthographic()` store that projection's parameters and rebuild the matrix, but do **not** set `m_ProjectionType`. Calling `SetOrthographic()` on a default (perspective) camera looks like it should switch modes and silently does nothing visible; the caller must also call `SetProjectionType()`. Either set the type inside the setters or rename them to `SetPerspectiveProps` / `SetOrthographicProps` so the contract is obvious.
- [Ember/src/Ember/Asset/AssetManager.h](Ember/src/Ember/Asset/AssetManager.h#L74) — `AssetManager::SaveAssetToFile<T>()` calls every serializer with its arguments **reversed**: it passes `(asset, absolutePath)` while `MeshSerializer`, `ModelSerializer`, `SkeletonSerializer`, `SkeletonMaskSerializer`, `PhysicsMaterialSerializer`, `AnimationSerializer` and `NavigationMeshSerializer` all declare `Serialize(const std::filesystem::path&, const SharedPtr<T>&)`. Only the `AnimationController` branch has them the right way round. Because `SaveAssetToFile` is a template with `if constexpr` branches, the broken branches are never instantiated today — the moment anything calls `SaveAssetToFile<Mesh>` (or any of the others) it is a **compile error**, not a runtime one. Swap the argument order at the call sites in `SaveAssetToFile`. Found while writing `Ember-Test/src/Tests/AssetTests.cpp`, which deliberately calls `PhysicsMaterialSerializer::Serialize` directly to avoid tripping it.

### Project / build pipeline

- [Ember/src/Ember/Core/ProjectManager.cpp](Ember/src/Ember/Core/ProjectManager.cpp#L52) — Copy default engine assets into a new project directory. Without this, every new project starts broken until you manually copy `DefaultSkybox.hdr`, default meshes, etc.
- ~~Account for build types (Debug/Release) when packaging a runtime build.~~ **Done** — `Paths::RuntimeExe()` (`Ember/src/Ember/Core/Paths.cpp`) owns the config-folder mapping for dev builds and returns `<install>/Ember-Runtime.exe` for an installed one, replacing the two duplicated `#if` chains in `ProjectManager.cpp` and `EditorLayer.cpp`. The old chains also mapped Profile to `Release-windows-x86_64`, since Profile defines `EB_RELEASE` too.
- [Ember/src/Ember/Core/ProjectManager.cpp](Ember/src/Ember/Core/ProjectManager.cpp#L216) — Architecture/system awareness in project paths. Still x64/Windows-only: `Paths::ConfigFolder()` hardcodes the `-windows-x86_64` suffix.

---

## P1 — High Impact for MVP

Things that won't *block* you but will repeatedly slow you down while making a game.

### Editor UX

- [Ember-Forge/src/EditorLayer.cpp](Ember-Forge/src/EditorLayer.cpp#L184) — Pause the runtime when the editor pauses. Essential for inspecting bugs mid-play.
- [Ember-Forge/src/EditorLayer.cpp](Ember-Forge/src/EditorLayer.cpp#L1085) — Spawn new entities at the cursor position rather than always at origin. Massive quality-of-life win when laying out a level.
- [Ember-Forge/src/Panels/AssetManagerPanel.cpp](Ember-Forge/src/Panels/AssetManagerPanel.cpp#L260) — Auto-select an item after creation/import. Small fix, large workflow win.
- [Ember-Forge/src/ComponentUI/Collision/ColliderComponentUI.h](Ember-Forge/src/ComponentUI/Collision/ColliderComponentUI.h#L149) — Default paths should point at the active project folder, not engine defaults. Otherwise asset pickers start in the wrong place every time.
- [Ember-Forge/src/ProjectSettingsDialog.cpp](Ember-Forge/src/ProjectSettingsDialog.cpp#L85) — Hook up the project-name field to real project data. Small but visible.

### Input / scripting (game logic)

- [Ember/src/Ember/Input/Input.h](Ember/src/Ember/Input/Input.h#L52) — Vector2 mouse position / scroll-offset accessors. Almost any game (camera, UI, aiming) needs this.
- [Ember/src/Ember/Script/Bindings/ScriptBindInput.cpp](Ember/src/Ember/Script/Bindings/ScriptBindInput.cpp#L147) — Move input functions into a proper `Input` Lua table. Cleaner script API → faster gameplay iteration.
- [Ember/src/Ember/ImGui/ImGuiLayer.h](Ember/src/Ember/ImGui/ImGuiLayer.h#L20) — Set up Key/Mouse events through the ImGui layer (needed if you want runtime UI / debug overlays to consume input correctly).
- [Ember/src/Ember/Event/Event.h](Ember/src/Ember/Event/Event.h#L33) — Mouse enter/exit/focus events. Required for any hover/tooltip UI in-game.

- **Runtime UI interaction** — DONE. `UIInputSystem` (`Ember/src/Ember/ECS/System/UIInputSystem.h`) is a
  centralized router in the shape of Unity's EventSystem: it CPU-raycasts UI rects, owns hover, pointer
  capture and focus, and drives `UISelectableComponent` + `UIButtonComponent`/`UIToggleComponent`.
  In-game hover no longer needs the GLFW enter/exit events above (it is derived by polling), though those
  are still worth adding for tooltips outside the UI system. Guarded by `Ember-Test/src/Tests/UITests.cpp`;
  authoring guide in `docs/Editor/BuildingUI.md`.

- **`LayerStack` event ordering** — still open, and now the last piece of the input story.
  `PushCanvasLayer` appends to the *end* of the stack while `Application::OnEvent` iterates front-to-back,
  so overlay layers receive events **last** — backwards for an overlay. `Event::Handled()` exists but
  nothing reads it, so no layer can consume input. `UIInputSystem` sidesteps this with the
  `UI.IsPointerOverUI()` query instead of event consumption. Separately, `LayerStack::PopLayer` decrements
  `m_LayerPartitionIndex` even when popping a canvas layer.

### Animation / audio (gameplay-facing)

- [Ember/src/Ember/ECS/System/AnimationSystem.cpp](Ember/src/Ember/ECS/System/AnimationSystem.cpp#L139) — Playback speed multiplier on animator (slo-mo, hit-stop, speed-ups — all common game effects).
- [Ember/src/Ember/Asset/Animation.h](Ember/src/Ember/Asset/Animation.h#L26), [AnimationSerializer.cpp:51](Ember/src/Ember/Asset/AnimationSerializer.cpp#L51), [AnimationSerializer.cpp:117](Ember/src/Ember/Asset/AnimationSerializer.cpp#L117) — Scale keyframes (load + save). Without them, any imported animation that scales (squash & stretch, weapon swaps, etc.) is incomplete.
- [Ember/src/Ember/Audio/AudioSource.cpp](Ember/src/Ember/Audio/AudioSource.cpp#L66) — Trigger-restart flag for audio sources. Re-triggering one-shot SFX is fundamental.

### ECS correctness / authoring

- [Ember/src/Ember/Scene/Scene.cpp](Ember/src/Ember/Scene/Scene.cpp#L199) — Pin down `OnAttach` vs `OnRuntimeStart` ordering for systems. Ambiguity here causes spooky bugs that only show up after a play/stop cycle.
- [Ember/src/Ember/ECS/Component/Components.h](Ember/src/Ember/ECS/Component/Components.h#L425) — Material name clash handling. Will bite when a level uses several variants of the same base material.

---

## P2 — Nice-to-Have for MVP

Quality / polish / scope-expansion. Pick up opportunistically.

### Rendering polish

- [Ember/src/Ember/Render/Pass/ShadowRenderPass.h](Ember/src/Ember/Render/Pass/ShadowRenderPass.h#L39) — Expose shadow params to UI (lets you tune without recompiling).
- [Ember/src/Ember/Render/Pass/ShadowRenderPass.h](Ember/src/Ember/Render/Pass/ShadowRenderPass.h#L42) — Move `m_BlendOverlap` to a uniform so editor + shader stay in sync.
- [Ember/src/Ember/Render/Pass/ShadowRenderPass.cpp](Ember/src/Ember/Render/Pass/ShadowRenderPass.cpp#L234), [line 242](Ember/src/Ember/Render/Pass/ShadowRenderPass.cpp#L242) — Multi-shadow-map array + cascade shadows for spotlights (only if your game leans on spotlights).
- [Ember/src/Ember/Render/VFX/VFXTypes.h](Ember/src/Ember/Render/VFX/VFXTypes.h#L28) — Per-volume LUT UUID + bake/save logic for color grading.
- [Ember/src/Ember/Render/Particle.h](Ember/src/Ember/Render/Particle.h#L29) — Sprite-sheet support for animated particles.
- [Ember/src/Ember/Render/Particle.h](Ember/src/Ember/Render/Particle.h#L30) — Velocity damping for thicker particles.
- [Ember-Forge/src/EditorLayer.cpp](Ember-Forge/src/EditorLayer.cpp#L808) — Orthographic mode in ImGuizmo for 2D scenes (only if you want a 2D game).

### Editor / asset workflow polish

- [Ember-Forge/src/UI/DragDropTypes.h](Ember-Forge/src/UI/DragDropTypes.h#L62) — Distinguish meshes vs models in drag-drop payloads.
- [Ember-Forge/src/Panels/EnvironmentPanel.cpp](Ember-Forge/src/Panels/EnvironmentPanel.cpp#L78) — Move skybox UUID to a constant.
- [Ember/src/Ember/Asset/AssetManager.cpp](Ember/src/Ember/Asset/AssetManager.cpp#L12) — Split default-asset loading into focused methods.
- [Ember/src/Ember/Render/Texture.h](Ember/src/Ember/Render/Texture.h#L24) — Integrate texture type into texture objects.
- [Ember/src/Ember/Platform/OpenGL/Texture2DArray.cpp](Ember/src/Ember/Platform/OpenGL/Texture2DArray.cpp#L24) — Wrap mode parameter for `Texture2DArray`.

---

## P3 — Post-MVP (defer)

Optimizations, internal refactors, debug-only conveniences, and "code organization" items. None of these block making a game.

### Refactors

- [Ember-Forge/src/EditorLayer.cpp](Ember-Forge/src/EditorLayer.cpp#L755) — Split editor debug-draw blocks into separate methods.
- [Ember/src/Ember/ECS/System/AISystem.h](Ember/src/Ember/ECS/System/AISystem.h#L38) — Split AI debug rendering into its own class.
- [Ember/src/Ember/ECS/System/AISystem.cpp](Ember/src/Ember/ECS/System/AISystem.cpp#L281) — Merge duplicated highlighted/unhighlighted segment render code.
- [Ember/src/Ember/ECS/System/CharacterControllerSystem.h.cpp](Ember/src/Ember/ECS/System/CharacterControllerSystem.h.cpp#L164) — Move code to clean up `Update`. (Also: rename file — `.h.cpp` is almost certainly a typo.)
- [Ember/src/Ember/ECS/System/RenderSystem.cpp](Ember/src/Ember/ECS/System/RenderSystem.cpp#L57) — Move one-shot setup out of `OnAttach`.
- [Ember/src/Ember/ECS/System/RenderSystem.h](Ember/src/Ember/ECS/System/RenderSystem.h#L70) — Turn render path into a render graph. Large refactor, ignore until P1/P2 is done.
- [Ember/src/Ember/Render/VFX/VFXTypes.h](Ember/src/Ember/Render/VFX/VFXTypes.h#L17) — Break color-grade settings into separate passes.
- [Ember/src/Ember/Render/Pass/PostProcessRenderPass.cpp](Ember/src/Ember/Render/Pass/PostProcessRenderPass.cpp#L141) — Move exposure to its own pass.
- [Ember/src/Ember/Render/Texture.h](Ember/src/Ember/Render/Texture.h#L77) — Move texture library here.
- [Ember/src/Ember/Render/Renderer2D.cpp](Ember/src/Ember/Render/Renderer2D.cpp#L98) — Add to shader/texture libraries.
- [Ember/src/Ember/Core/Logger.cpp](Ember/src/Ember/Core/Logger.cpp#L17) — Customizable logger abstraction.
- [Ember/src/Ember/Core/FilterManager.h](Ember/src/Ember/Core/FilterManager.h#L12) — Unify `RenderLayer` and `CollisionFilter` into one `Filter`.
- [Ember/src/Ember/Core/Constants.h](Ember/src/Ember/Core/Constants.h#L30) — Re-section constants into large numeric ranges.

### Optimizations

- [Ember/src/Ember/ECS/View.h](Ember/src/Ember/ECS/View.h#L7) — Dynamic sorting by smallest component-array type in views.
- [Ember/src/Ember/ECS/System/AnimationSystem.cpp](Ember/src/Ember/ECS/System/AnimationSystem.cpp#L68) — Bone-to-track cache.
- [Ember/src/Ember/ECS/System/LifecycleSystem.cpp](Ember/src/Ember/ECS/System/LifecycleSystem.cpp#L14) — Avoid repeat vector resizing for entity removal.
- [Ember/src/Ember/ECS/Component/Components.h](Ember/src/Ember/ECS/Component/Components.h#L783) — Shared-ptr nav nodes to reduce copying.
- [Ember/src/Ember/Scene/Scene.h](Ember/src/Ember/Scene/Scene.h#L126) — Entity-name lookup map. Cheap and decently useful — promote to P2 if you find yourself searching by name a lot in scripts.
- [Ember/src/Ember/Math/Math.h](Ember/src/Ember/Math/Math.h#L187) — Custom matrix decompose to skip `glm::decompose` overhead. Pair with the P0 NaN fix above.
- [Ember/src/Ember/Asset/Prefab.h](Ember/src/Ember/Asset/Prefab.h#L24) — Store prefab YAML as binary.
- [Ember/src/Ember/Render/Pass/BillboardsRenderPass.cpp](Ember/src/Ember/Render/Pass/BillboardsRenderPass.cpp#L40) — Remove a redundant uniform already provided via UBO.

### Misc / cleanup

- [Ember/src/Ember/Core/Layer.h](Ember/src/Ember/Core/Layer.h#L58) — Make `m_Name` debug-only.
- [Ember/src/Ember/Core/Pool.h](Ember/src/Ember/Core/Pool.h#L29) — Investigate / remove a probably-unneeded teardown step.
- [Ember/src/Ember/ECS/System/PhysicsSystem.h](Ember/src/Ember/ECS/System/PhysicsSystem.h#L97) — Make `m_PhysicsWorld` a scoped pointer.
- [Ember/src/Ember/Physics/PhysicsEventListener.h](Ember/src/Ember/Physics/PhysicsEventListener.h#L18) — Implement extra event hooks when a use case arises (intentionally deferred).

---

## Suggested Roadmap to "Start Your First Game"

1. **Sprint 1 — Stop the bleeding (P0).** Fix the transform-decompose NaN, the entity-children bug, animation-event serialization, and project-template/asset-copy. Once these land, the editor becomes safe to *trust*.
2. **Sprint 2 — Gameplay surface area (P1 gameplay).** Vector2 mouse input + Lua `Input` table + audio retrigger + animation playback speed + animation scale keys. This is the minimum to script real game feel.
3. **Sprint 3 — Editor flow (P1 editor).** Runtime-pause sync, spawn-at-cursor, asset-panel auto-select, project-local default paths. Laying out a level becomes pleasant.
4. **Start the game.** Tackle P2 items only when they start blocking you in practice; let real-game pain prioritize the rest. Treat P3 as "engine v2" work.
