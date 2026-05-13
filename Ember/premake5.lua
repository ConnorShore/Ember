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
      "vendor/reactphysics3d/include"
   }

   files 
   { 
      "src/**.h",
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

   filter "configurations:Debug"
      defines { "EB_DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "EB_RELEASE" }
      optimize "On"

   filter "configurations:Dist"
      defines { "EB_DIST" }
      runtime "Release"
      staticruntime "On"
      optimize "On"
      symbols "Off"

   filter {}