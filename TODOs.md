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

### Bugs

- [Ember/src/Ember/Scene/Entity.cpp](Ember/src/Ember/Scene/Entity.cpp#L8) — `GetAllChildren()` may be constructing new `Entity` objects rather than fetching from the scene registry. Parenting / hierarchy traversal is core; any subtle bug here will surface as ghost entities, orphaned references, or broken prefab updates.
- [Ember-Forge/src/EditorLayer.cpp](Ember-Forge/src/EditorLayer.cpp#L858) — Transform decompose produces NaNs on negative scaling. **Guaranteed to bite you** the moment you flip a mesh in the editor or import any mirrored asset. Either fix the math or detect negative scales and route through a different path.
- [Ember/src/Ember/Asset/AnimationSerializer.cpp](Ember/src/Ember/Asset/AnimationSerializer.cpp#L29) — "Test event serialization when scene is saved." If animation events aren't round-tripping through save/load, scripted gameplay tied to animations will silently fail.

### Project / build pipeline

- [Ember/src/Ember/Core/ProjectManager.cpp](Ember/src/Ember/Core/ProjectManager.cpp#L52) — Copy default engine assets into a new project directory. Without this, every new project starts broken until you manually copy `DefaultSkybox.hdr`, default meshes, etc.
- [Ember/src/Ember/Core/ProjectManager.cpp](Ember/src/Ember/Core/ProjectManager.cpp#L136) — Account for build types (Debug/Release) and platforms when packaging a runtime build. Required to actually *ship* the game.
- [Ember/src/Ember/Core/ProjectManager.cpp](Ember/src/Ember/Core/ProjectManager.cpp#L127) — Architecture/system awareness in project paths (paired with the above).

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
