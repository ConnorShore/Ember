workspace "Ember"
   architecture "x86_64"
   configurations { "Debug", "Release" }
   cppdialect "C++23"
   startproject "Ember-Forge"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "Ember/vendor/GLFW"
include "Ember/vendor/glad"
include "Ember/vendor/imgui"
include "Ember/vendor/assimp"

include "Ember-Forge/vendor/ImGuizmo"

include "Ember"
include "Ember-Forge"
include "Sandbox"