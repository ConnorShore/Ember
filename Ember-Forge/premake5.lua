project "Ember-Forge"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++23"

   targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
   objdir ("%{wks.location}/bin/int/" .. outputdir .. "/%{prj.name}")
   debugdir "%{wks.location}"

   pchheader "efpch.h"
   pchsource "src/efpch.cpp"

   multiprocessorcompile "On"

   defines
   {
      "EB_EDITOR",
   }

   files
   { 
      "src/**.h",
      "src/**.cpp" 
   }

   includedirs 
   {
      "src",
      "vendor/ImGuizmo",
      "vendor/imgui-node-editor",
      "%{wks.location}/Ember/src",
      "%{wks.location}/Ember/vendor",
      "%{wks.location}/Ember/vendor/imgui",
      "%{wks.location}/Ember/vendor/glm",
      "%{wks.location}/Ember/vendor/stb",
      "%{wks.location}/Ember/vendor/miniaudio",
      "%{wks.location}/Ember/vendor/lua/src",
      "%{wks.location}/Ember/vendor/sol2/include",
      "%{wks.location}/Ember/vendor/rapidyaml/src",
      "%{wks.location}/Ember/vendor/reactphysics3d/include",
	  "%{wks.location}/Ember/vendor/recastnavigation/Recast/Include",
	  "%{wks.location}/Ember/vendor/recastnavigation/Detour/Include",
        "%{wks.location}/Ember/vendor/recastnavigation/DetourTileCache/Include",
        "%{wks.location}/Ember/vendor/recastnavigation/DebugUtils/Include",
		"%{wks.location}/Ember/vendor/rapidyaml/ext/c4core/src",
      "%{wks.location}/Ember-Tools/src",
      "%{wks.location}/Ember-Tools/vendor/tinygltf",
   }

   links 
   {
      "Ember",
      "Ember-Runtime",
      "Ember-Tools",
      "ImGuizmo",
      "imgui-node-editor"
   }

   filter "system:windows"
      systemversion "latest"
      -- Embeds EmberIcon.ico so the executable itself is branded, not just the installer's shortcuts.
      files { "EmberForge.rc" }

   -- Make "uninitialized variable used" a hard build error (C4700 definitely, C4701/C4703
   -- potentially) so the optimizer-exposed UB behind the Debug-vs-Release rendering divergence
   -- fails the build at the exact line. `/w1XXXX` force-enables the /W4-level ones; `/weXXXX`
   -- promotes them to errors. Scoped to engine sources so vendored TUs are unaffected.
   filter { "system:windows", "files:src/**.cpp" }
      buildoptions { "/w14700", "/w14701", "/w14703", "/we4700", "/we4701", "/we4703" }

   -- The inspector draws every component type, so it instantiates the ECS templates once per
   -- component and blows past the COFF section limit in Debug - same reason as the script binders.
   filter { "system:windows", "files:src/Panels/Inspector/**.cpp" }
      buildoptions { "/bigobj" }

   filter "configurations:Debug"
      defines { "EB_DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "EB_RELEASE" }
      optimize "On"

   filter "configurations:Profile"
      defines { "EB_PROFILE", "EB_RELEASE" }
      optimize "On"
      symbols "On"

   filter "configurations:Dist"
      defines { "EB_DIST" }
      runtime "Release"
      staticruntime "On"
      optimize "On"
      symbols "Off"
      -- No console window for the shipped editor; entrypoint is needed because EntryPoint.h
      -- defines main() rather than WinMain(). Logging goes to a file instead (see EmberForgeApp).
      kind "WindowedApp"
      entrypoint "mainCRTStartup"

   filter {}