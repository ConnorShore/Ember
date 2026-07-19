project "Ember-Tools"
   kind "StaticLib"
   language "C++"
   cppdialect "C++23"

   targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
   objdir ("%{wks.location}/bin/int/" .. outputdir .. "/%{prj.name}")
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
      "%{wks.location}/Ember-Tools/vendor/tinygltf",
      "%{wks.location}/Ember/src",
      "%{wks.location}/Ember/vendor",
      "%{wks.location}/Ember/vendor/glm",
      "%{wks.location}/Ember/vendor/stb",
      "%{wks.location}/Ember/vendor/lua/src",
      "%{wks.location}/Ember/vendor/sol2/include",
      "%{wks.location}/Ember/vendor/rapidyaml/src",
	  "%{wks.location}/Ember/vendor/recastnavigation/Recast/Include",
	  "%{wks.location}/Ember/vendor/recastnavigation/Detour/Include",
		"%{wks.location}/Ember/vendor/rapidyaml/ext/c4core/src"
   }

   links 
   {
      "Ember",
   }

   filter "system:windows"
      systemversion "latest"

   -- Make "uninitialized variable used" a hard build error (C4700 definitely, C4701/C4703
   -- potentially) so the optimizer-exposed UB behind the Debug-vs-Release rendering divergence
   -- fails the build at the exact line. `/w1XXXX` force-enables the /W4-level ones; `/weXXXX`
   -- promotes them to errors. Scoped to engine sources so vendored TUs are unaffected.
   filter { "system:windows", "files:src/**.cpp" }
      buildoptions { "/w14700", "/w14701", "/w14703", "/we4700", "/we4701", "/we4703" }

   filter "configurations:Debug"
      defines { "EB_DEBUG", "EB_PROFILE" }
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