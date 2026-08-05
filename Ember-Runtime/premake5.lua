project "Ember-Runtime"
   language "C++"
   cppdialect "C++23"
   kind "ConsoleApp"

   targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
   objdir ("%{wks.location}/bin/int/" .. outputdir .. "/%{prj.name}")
   debugdir "%{wks.location}"

--    pchheader "efpch.h"
--    pchsource "src/efpch.cpp"

   multiprocessorcompile "On"

   files
   { 
      "src/**.h",
      "src/**.cpp" 
   }

   includedirs 
   {
      "src",
      "%{wks.location}/Ember/src",
      "%{wks.location}/Ember/vendor",
      "%{wks.location}/Ember/vendor/glm",
      "%{wks.location}/Ember/vendor/stb",
      "%{wks.location}/Ember/vendor/miniaudio",
      "%{wks.location}/Ember/vendor/lua/src",
      "%{wks.location}/Ember/vendor/sol2/include",
      "%{wks.location}/Ember/vendor/rapidyaml/src",
      "%{wks.location}/Ember/vendor/reactphysics3d/include",
	  "%{wks.location}/Ember/vendor/recastnavigation/Recast/Include",
	  "%{wks.location}/Ember/vendor/recastnavigation/Detour/Include",
	  "%{wks.location}/Ember/vendor/rapidyaml/ext/c4core/src"
   }

   links 
   {
      "Ember"
   }

   filter "system:windows"
      systemversion "latest"
      -- Embeds EmberIcon.ico so exported games carry the brand icon, not the generic Windows one.
      files { "EmberRuntime.rc" }

   -- Make "uninitialized variable used" a hard build error (C4700 definitely, C4701/C4703
   -- potentially) so the optimizer-exposed UB behind the Debug-vs-Release rendering divergence
   -- fails the build at the exact line. `/w1XXXX` force-enables the /W4-level ones; `/weXXXX`
   -- promotes them to errors. Scoped to engine sources so vendored TUs are unaffected.
   filter { "system:windows", "files:src/**.cpp" }
      buildoptions { "/w14700", "/w14701", "/w14703", "/we4700", "/we4701", "/we4703" }

   filter "configurations:Debug"
      defines { "EB_DEBUG" }
      symbols "On"
      -- kind "ConsoleApp" 

   filter "configurations:Release"
      defines { "EB_RELEASE" }
      optimize "On"
      -- kind "WindowedApp"
      -- entrypoint "mainCRTStartup"

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
      -- No console window for a shipped game; entrypoint is needed because EntryPoint.h defines
      -- main() rather than WinMain(). Logging still reaches Logs/runtime.txt (see EmberRuntimeApp).
      kind "WindowedApp"
      entrypoint "mainCRTStartup"

   filter {}