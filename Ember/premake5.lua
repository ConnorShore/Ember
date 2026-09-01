project "Ember"
   kind "StaticLib"
   language "C++"
   cppdialect "C++23"

   targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
   objdir ("%{wks.location}/bin/int/" .. outputdir .. "/%{prj.name}")

   pchheader "ebpch.h"
   pchsource "src/ebpch.cpp"

   multiprocessorcompile "On"

   includedirs 
   {
      "src",
      "vendor/GLFW/include",
      "vendor/glad/include",
      "vendor/glm",
      "vendor/imgui",
      "vendor/stb",
      "vendor/miniaudio",
      "vendor/rapidyaml/src",
		"vendor/rapidyaml/ext/c4core/src",
      "vendor/lua/src",
      "vendor/sol2/include",
      "vendor/reactphysics3d/include",
      "vendor/recastnavigation/Recast/Include",
      "vendor/recastnavigation/Detour/Include",
      "vendor/recastnavigation/DetourTileCache/Include",
      "vendor/recastnavigation/DebugUtils/Include"
   }

   files 
   { 
      "src/**.h",
      "src/**.inl",
      "src/**.cpp",
      "vendor/stb/**.h",
      "vendor/stb/**.cpp",
      "vendor/miniaudio/**.h",
      "vendor/miniaudio/**.cpp"
   }

   links
   {
      "GLFW",
      "glad",
      "imgui",
      "rapidyaml",
      "lua",
      "reactphysics3d",
      "recastnavigation",
      "opengl32.lib",
   }

   defines
   {
      "EB_ENGINE",
	   "_CRT_SECURE_NO_WARNINGS",
      "GLFW_INCLUDE_NONE",
      "SOL_ALL_SAFETIES_ON=1"
   }

   filter "files:vendor/stb/**.cpp or files:vendor/miniaudio/**.cpp"
      enablepch "Off"

   filter "system:windows"
      systemversion "latest"

   -- Make "uninitialized variable used" a hard build error (C4700 definitely, C4701/C4703
   -- potentially) so the optimizer-exposed UB behind the Debug-vs-Release rendering divergence
   -- fails the build at the exact line. `/w1XXXX` force-enables the /W4-level ones; `/weXXXX`
   -- promotes them to errors. Scoped to engine sources so vendored TUs (stb/miniaudio) are unaffected.
   filter { "system:windows", "files:src/**.cpp" }
      buildoptions { "/w14700", "/w14701", "/w14703", "/we4700", "/we4701", "/we4703" }

   -- The script binders instantiate a sol2 usertype per component and blow past the COFF section
   -- limit in Debug, where every template gets its own section.
   filter { "system:windows", "files:src/Ember/Script/**.cpp" }
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

   filter {}