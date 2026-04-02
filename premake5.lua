workspace "Ember"
   architecture "x86_64"
   configurations { "Debug", "Release" }
   cppdialect "C++23"
   startproject "Ember-Forge"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "Ember/vendor/GLFW"
include "Ember/vendor/glad"
include "Ember/vendor/imgui"
include "Ember/vendor/rapidyaml"
include "Ember/vendor/lua"

include "Ember-Forge/vendor/ImGuizmo"

include "Ember-Tools/vendor/assimp"

include "Ember"
include "Ember-Tools"
include "Ember-Forge"