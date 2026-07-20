workspace "Ember"
   architecture "x86_64"
   configurations { "Debug", "Release", "Profile", "Dist" }
   cppdialect "C++23"
   startproject "Ember-Forge"

   -- glm leaves vec/mat/quat UNINITIALIZED on default construction; force value-initialization
   -- engine-wide so a bare `Vector3f x;` / `Matrix4f m;` is zero/identity instead of Release-only
   -- garbage. Must be workspace-scoped: defining it in only some glm-including TUs is an ODR hazard.
   defines { "GLM_FORCE_CTOR_INIT" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "Ember/vendor/GLFW"
include "Ember/vendor/glad"
include "Ember/vendor/imgui"
include "Ember/vendor/rapidyaml"
include "Ember/vendor/lua"
include "Ember/vendor/reactphysics3d"
include "Ember/vendor/recastnavigation"

include "Ember-Forge/vendor/ImGuizmo"
include "Ember-Forge/vendor/imgui-node-editor"

include "Ember"
include "Ember-Runtime"
include "Ember-Tools"
include "Ember-Test"
include "Ember-Forge"