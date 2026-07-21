project "Ember-Test"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++23"

   targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
   objdir ("%{wks.location}/bin/int/" .. outputdir .. "/%{prj.name}")
   -- Run from the workspace root so relative asset paths (Ember/assets, Ember-Test/golden) resolve.
   debugdir "%{wks.location}"

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

   -- Match the engine projects: uninitialized-variable warnings are hard errors (scoped to our own
   -- sources so vendored TUs are unaffected). Keeps test code held to the same standard.
   filter { "system:windows", "files:src/**.cpp" }
      buildoptions { "/w14700", "/w14701", "/w14703", "/we4700", "/we4701", "/we4703" }

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

   filter {}
