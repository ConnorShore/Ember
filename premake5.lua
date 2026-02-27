workspace "Ember"
   architecture "x86_64"
   configurations { "Debug", "Release" }
   cppdialect "C++23"
   startproject "Sandbox"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "Ember"
include "Sandbox"