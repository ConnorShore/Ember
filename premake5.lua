workspace "Ember"
   architecture "x86_64"
   configurations { "Debug", "Release", "Dist" }
   cppdialect "C++23"
   startproject "Ember-Forge"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "Ember/vendor/GLFW"
include "Ember/vendor/glad"
include "Ember/vendor/imgui"
include "Ember/vendor/rapidyaml"
include "Ember/vendor/lua"
include "Ember/vendor/reactphysics3d"

include "Ember-Forge/vendor/ImGuizmo"
include "Ember-Forge/vendor/imgui-node-editor"

include "Ember"
include "Ember-Runtime"
include "Ember-Tools"
include "Ember-Forge"