# Ember Custom Shader Reference

Ember's renderer compiles **GLSL 4.5 core** shader assets through a small custom parser that:

1. Splits a single `.glsl` file into vertex/fragment stages (and optionally geometry/compute).
2. Scans `// @UIProperty(...)` annotations so uniforms become editable widgets in the inspector.
3. Injects engine-supplied `#define` macros after each `#version` line.
4. Watches the file on disk and **hot-reloads** any non-engine shader whose source changes at runtime.

This document is the reference for authoring those shaders. It mirrors the behavior of
[`Ember/src/Ember/Render/ShaderParser.cpp`](../Ember/src/Ember/Render/ShaderParser.cpp) and the
preset templates emitted by the **Material → Create Shader** editor workflow.

---

## Table of Contents

- [File Layout](#file-layout)
- [Creating a Shader from the Editor](#creating-a-shader-from-the-editor)
- [Render Queues](#render-queues)
  - [Opaque (Deferred / G-Buffer)](#opaque-deferred--g-buffer)
  - [Forward](#forward)
  - [Transparent](#transparent)
- [Vertex Attribute Locations](#vertex-attribute-locations)
- [Engine-Supplied Uniforms](#engine-supplied-uniforms)
- [Exposing Properties to the Editor](#exposing-properties-to-the-editor)
  - [`// @UIProperty` Syntax](#-uiproperty-syntax)
  - [Property Types](#property-types)
  - [Default Values](#default-values)
  - [Texture Bindings](#texture-bindings)
- [Macros](#macros)
- [Hot Reload](#hot-reload)
- [Fallback Shader](#fallback-shader)
- [Worked Examples](#worked-examples)
  - [Forward unlit emissive](#forward-unlit-emissive)
  - [Opaque PBR](#opaque-pbr)
  - [Transparent water](#transparent-water)
- [Tips & Gotchas](#tips--gotchas)

---

## File Layout

A shader is a single `.glsl` file that uses `#shader` directives to mark each stage. Anything
between two `#shader` lines (or between the last `#shader` and end-of-file) belongs to that stage.

```glsl
#shader vertex
#version 450 core

// vertex code...

#shader fragment
#version 450 core

// fragment code...
```

Recognized stage names: `vertex`, `fragment`. Each stage must declare its own `#version` line —
the parser does not duplicate it for you.

---

## Creating a Shader from the Editor

The fastest way to start is from the **Material Component** in the Inspector:

1. Add a `MaterialComponent` to an entity and click **Create** to make a new material.
2. With the material selected and no shader assigned, the **Shader** combo shows an `Add` button.
3. Click **Add** to open the *Create New Shader* modal. Pick a name and a **Render Queue**.
4. Click **Create** — a templated `.glsl` is written to `<Project>/Assets/Shaders/<Name>.glsl`,
   loaded as a non-engine `Shader` asset, assigned to the material, and opened in VS Code.

The generated template already contains the correct outputs for the queue you picked and a few
`// @UIProperty` uniforms with sensible defaults so the material renders something visible
immediately after creation. The first frame after `SetShader` (or after hot reload introduces new
properties), the material panel seeds defaults for any uniform it doesn't yet have — see
[Default Values](#default-values).

---

## Render Queues

Each material declares a `RenderQueue` which controls **which framebuffer the shader writes into**
and how the engine composites the result. The fragment shader's `layout(location = N) out` list
**must** match the queue's expected attachments — using the wrong layout is the most common cause
of black or transparent surfaces on new shaders.

### Opaque (Deferred / G-Buffer)

Opaque materials write into the deferred G-buffer. The engine's lighting pass reads these targets
to compute lit color later. The fragment shader **does not** apply lighting itself.

```glsl
layout(location = 0) out vec4 AlbedoRoughness;   // rgb = albedo,  a = roughness
layout(location = 1) out vec4 NormalMetallic;    // rgb = world-space normal, a = metallic
layout(location = 2) out vec4 PositionAO;        // rgb = world position,     a = ambient occlusion
layout(location = 3) out vec4 EmissionOut;       // rgb = emissive, a = unused
layout(location = 4) out int  EntityID;          // engine-managed picking buffer
```

> Set `EntityID = u_EntityID;` (see [Engine-Supplied Uniforms](#engine-supplied-uniforms)) so the
> editor's entity picking continues to work.

### Forward

Forward materials shade directly to the lit color target and contribute to bloom. Use this for
self-lit / unlit effects, emissive props, decals, etc.

```glsl
layout(location = 0) out vec4 OutColor;      // lit color (rgb), alpha typically 1.0
layout(location = 1) out vec4 BrightColor;   // pixels above 1.0 luminance for the bloom pass
layout(location = 2) out int  EntityID;
```

Typical emissive extraction:

```glsl
BrightColor = vec4(max(OutColor.rgb - vec3(1.0), vec3(0.0)), 1.0);
```

### Transparent

Same attachments and layout as `Forward`, but the renderer enables alpha blending and depth-write
is disabled. Write the alpha channel of `OutColor` to control the surface's opacity.

```glsl
layout(location = 0) out vec4 OutColor;
layout(location = 1) out vec4 BrightColor;
layout(location = 2) out int  EntityID;

void main()
{
    OutColor = vec4(color, opacity);   // alpha drives the blend
    BrightColor = vec4(0.0);
    EntityID = u_EntityID;
}
```

---

## Vertex Attribute Locations

The engine binds mesh data to fixed attribute locations. Declare only the ones you need:

| Location | Attribute | Type | Notes |
| --- | --- | --- | --- |
| `0` | `v_Position` | `vec3` | Object-space position. Always present. |
| `1` | `v_Normal` | `vec3` | Object-space normal. |
| `2` | `v_TextureCoord` | `vec2` | UV0. |
| `3` | `v_Tangent` | `vec3` | Object-space tangent (may be zero if the mesh didn't ship one — see `StandardGeometry.glsl` for a robust fallback). |
| `4` | `v_Bitangent` | `vec3` | Object-space bitangent. |

Skinned meshes add bone indices/weights at additional locations — see
`Ember/assets/shaders/StandardGeometrySkinned.glsl` if you need those.

---

## Engine-Supplied Uniforms

The renderer uploads these every frame. Declare the ones you use exactly as shown.

### Camera UBO (binding 0)

```glsl
layout(std140, binding = 0) uniform CameraData
{
    mat4 u_ViewProjection;
};
```

### Per-draw uniforms

| Uniform | Type | Set by |
| --- | --- | --- |
| `u_Transform` | `mat4` | The entity's world transform. Multiply your `v_Position` by this in the vertex shader. |
| `u_EntityID` | `int` | The picking ID for this entity. Write it to the `EntityID` framebuffer output to keep selection working. |

### Material uniforms

Anything else declared as `uniform` is treated as a **material uniform**. The material owns its
value and uploads it on `Bind()`. Pair these with a `// @UIProperty` annotation to expose them in
the inspector.

---

## Exposing Properties to the Editor

### `// @UIProperty` Syntax

A `// @UIProperty(...)` comment on the line **immediately above** a `uniform` declaration tells
the parser to register that uniform as an editor property:

```glsl
// @UIProperty(Name = "Albedo", Type = Color3)
uniform vec3 u_Albedo;

// @UIProperty(Name = "Roughness", Type = Slider, Min = 0.0, Max = 1.0)
uniform float u_Roughness;
```

The annotation arguments are `Key = Value` pairs, comma-separated, all optional except `Type`.
String values may be wrapped in double quotes.

| Key | Applies to | Description |
| --- | --- | --- |
| `Name` | All | Display name in the inspector. Defaults to the uniform name with the `u_` prefix kept. |
| `Type` | All (required) | One of the [Property Types](#property-types) below. |
| `Min` | `Float`, `Slider`, vector types | Lower bound for the widget and for normalization. |
| `Max` | `Float`, `Slider`, vector types | Upper bound. |
| `Step` | Numeric drag widgets | Drag sensitivity (default `0.005`). |
| `Normalize` | Numeric | If `true`, the editor remaps the user-entered `[Min, Max]` value into `[0, 1]` before storing it on the material. |

### Property Types

| `Type` | GLSL uniform | Editor widget |
| --- | --- | --- |
| `Float` | `float` | Drag float (uses `Min`/`Max`/`Step`). |
| `Float2` | `vec2` | Two drag floats. |
| `Float3` | `vec3` | Three drag floats. |
| `Float4` | `vec4` | Four drag floats. |
| `Color3` | `vec3` | Color picker (no alpha). |
| `Color4` | `vec4` | Color picker with alpha. |
| `Slider` | `float` | Slider in `[Min, Max]`. |
| `Texture` | `sampler2D` | Drag-and-drop texture slot. |

### Default Values

When the editor first encounters a property whose uniform isn't yet stored on the material, it
seeds a sensible default so the widget appears and the GPU doesn't read zeros. This also runs
after hot-reload, so newly-introduced properties get a default automatically.

| Property type | Seeded default |
| --- | --- |
| `Float` | `0.0` |
| `Slider` | midpoint of `[Min, Max]` |
| `Float2` / `Float3` / `Float4` | zero vector |
| `Color3` / `Color4` | white (`1, 1, 1[, 1]`) |
| `Texture` | A default 1×1 fallback texture chosen by uniform name: |
| | • names containing `normal` or `bump` → flat-normal (`0.5, 0.5, 1.0`) texture |
| | • names containing `emiss` or `ao` → black texture |
| | • everything else → white texture |

### Texture Bindings

Texture sampler slots are assigned **in the order textures appear in your `// @UIProperty` list**
(not by `layout(binding = ...)`). At draw time the material walks `Shader::GetProperties()`,
binds each `ShaderPropertyType::Texture` to the next free unit, and uploads its sampler `int` —
so as long as your annotations are in a stable order, the binding slot is deterministic.

```glsl
// Slot 0
// @UIProperty(Name = "Albedo Map", Type = Texture)
uniform sampler2D u_AlbedoTex;

// Slot 1
// @UIProperty(Name = "Normal Map", Type = Texture)
uniform sampler2D u_NormalTex;
```

---

## Macros

Engine code can pass a `ShaderMacros` map when loading a shader. Each entry becomes a `#define`
injected **immediately after every `#version` line** in the file, so your code can branch on it:

```glsl
#shader fragment
#version 450 core
// (parser inserts injected defines here)

#if MAX_BONES > 0
    // skinned path
#endif
```

User-authored project shaders are loaded without macros by default. The built-in engine shaders
use macros for things like `MAX_BONES`, `MAX_DIRECTIONAL_LIGHTS`, and `INVALID_ENTITY_ID`.

---

## Hot Reload

When a non-engine shader's `.glsl` file changes on disk:

1. The editor's per-frame `AssetManager::PollShaderHotReload()` notices the new
   `last_write_time` and calls `Shader::Reload()`.
2. The shader is re-parsed (including macros and `// @UIProperty` annotations).
3. The GPU program is recompiled and relinked. If compile/link **fails**, the shader's program id
   stays `0` and binds substitute the [fallback shader](#fallback-shader) (solid pink) instead of
   leaving the previous program active.
4. Any *new* `// @UIProperty` uniforms get seeded with defaults on the next material panel
   render (see [Default Values](#default-values)).

> Engine shaders that ship with the executable are intentionally skipped by the hot-reload poller.

To trigger a reload, just save the file. There's no editor button.

---

## Fallback Shader

`Ember/assets/shaders/Fallback.glsl` is a regular Shader asset — loaded by `AssetManager::LoadDefaults`
with `UUID = 8`, parsed and compiled like every other shader. It renders solid pink and is the
substitute used whenever another shader fails to compile or link. If you see pink on a surface,
check the editor log for the actual GLSL compile error.

---

## Worked Examples

### Forward unlit emissive

```glsl
#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;

layout(std140, binding = 0) uniform CameraData { mat4 u_ViewProjection; };
uniform mat4 u_Transform;

void main()
{
    gl_Position = u_ViewProjection * u_Transform * vec4(v_Position, 1.0);
}

#shader fragment
#version 450 core

layout(location = 0) out vec4 OutColor;
layout(location = 1) out vec4 BrightColor;
layout(location = 2) out int  EntityID;

// @UIProperty(Name = "Color", Type = Color3)
uniform vec3 u_Color;

// @UIProperty(Name = "Emission", Type = Float, Min = 0.0, Max = 50.0, Step = 0.05)
uniform float u_Emission;

uniform int u_EntityID;

void main()
{
    vec3 finalColor = u_Color * u_Emission;
    OutColor = vec4(finalColor, 1.0);
    BrightColor = vec4(max(OutColor.rgb - vec3(1.0), vec3(0.0)), 1.0);
    EntityID = u_EntityID;
}
```

### Opaque PBR

```glsl
#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TextureCoord;

layout(std140, binding = 0) uniform CameraData { mat4 u_ViewProjection; };
uniform mat4 u_Transform;

out vec2 TexCoord;
out vec3 WorldNormal;
out vec3 WorldPosition;

void main()
{
    TexCoord = v_TextureCoord;
    WorldNormal = mat3(u_Transform) * v_Normal;
    vec4 worldPos = u_Transform * vec4(v_Position, 1.0);
    WorldPosition = worldPos.xyz;
    gl_Position = u_ViewProjection * worldPos;
}

#shader fragment
#version 450 core

in vec2 TexCoord;
in vec3 WorldNormal;
in vec3 WorldPosition;

layout(location = 0) out vec4 AlbedoRoughness;
layout(location = 1) out vec4 NormalMetallic;
layout(location = 2) out vec4 PositionAO;
layout(location = 3) out vec4 EmissionOut;
layout(location = 4) out int  EntityID;

// @UIProperty(Name = "Albedo", Type = Color3)
uniform vec3 u_Albedo;

// @UIProperty(Name = "Roughness", Type = Slider, Min = 0.0, Max = 1.0)
uniform float u_Roughness;

// @UIProperty(Name = "Metallic", Type = Slider, Min = 0.0, Max = 1.0)
uniform float u_Metallic;

// @UIProperty(Name = "Albedo Map", Type = Texture)
uniform sampler2D u_AlbedoMap;

uniform int u_EntityID;

void main()
{
    vec3 albedo = u_Albedo * texture(u_AlbedoMap, TexCoord).rgb;
    AlbedoRoughness = vec4(albedo, u_Roughness);
    NormalMetallic  = vec4(normalize(WorldNormal), u_Metallic);
    PositionAO      = vec4(WorldPosition, 1.0);
    EmissionOut     = vec4(0.0);
    EntityID        = u_EntityID;
}
```

### Transparent water

```glsl
#shader fragment
#version 450 core

in vec3 WorldPosition;

layout(location = 0) out vec4 OutColor;
layout(location = 1) out vec4 BrightColor;
layout(location = 2) out int  EntityID;

// @UIProperty(Name = "Tint", Type = Color3)
uniform vec3 u_Tint;

// @UIProperty(Name = "Opacity", Type = Slider, Min = 0.0, Max = 1.0)
uniform float u_Opacity;

// @UIProperty(Name = "Wave Speed", Type = Float, Min = 0.0, Max = 5.0, Step = 0.05)
uniform float u_WaveSpeed;

uniform int u_EntityID;

void main()
{
    // ...wave/displacement math...
    OutColor    = vec4(u_Tint, u_Opacity);
    BrightColor = vec4(0.0);
    EntityID    = u_EntityID;
}
```

---

## Tips & Gotchas

- **Place `// @UIProperty` on the line directly above the `uniform`.** Blank lines or other
  comments between them break the association — the parser only consumes the *next* `uniform`
  line after seeing the annotation.
- **Don't `#version` more than once per stage.** Each stage's first non-`#shader` line should be
  its own `#version 450 core`. The parser doesn't reorder anything.
- **A pink mesh means the shader failed to compile.** Check the log. The fallback substitutes
  automatically; you don't need to revert anything.
- **Match the queue's output layout exactly.** A forward shader writing G-buffer outputs (or
  vice versa) typically renders black/transparent because the writes go to the wrong attachments.
- **Always write `u_EntityID`** to the entity-id output so picking keeps working.
- **Hot reload only watches non-engine shaders.** Editing files under `Ember/assets/shaders/`
  while the editor is running won't trigger a reload of the in-process program; rebuild the engine
  or copy the file into your project's `Assets/Shaders/` folder during development.
- **Texture slots are assigned by annotation order**, not `layout(binding=...)`. Keep the order
  consistent if you serialize material-uniform overrides.
- **The asset UUID is persistent across reloads.** Once a shader is created and saved into the
  project's asset registry, materials referencing it survive renames of the file as long as the
  `.eba` registry entry isn't deleted.
