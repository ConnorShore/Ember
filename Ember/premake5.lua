project "Ember"
   kind "StaticLib"
   language "C++"
   cppdialect "C++23"

   targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
   objdir ("%{wks.location}/bin/int/" .. outputdir .. "/%{prj.name}")

   pchheader "ebpch.h"
   pchsource "src/ebpch.cpp"

   includedirs 
   {
      "src",
      "vendor/GLFW/include",
      "vendor/glad/include",
      "vendor/glm",
      "vendor/imgui",
      "vendor/stb",
      "vendor/assimp/include",
      "vendor/rapidyaml/src",
		"vendor/rapidyaml/ext/c4core/src",
      "vendor/lua/src",
      "vendor/sol2/include"
   }

   files 
   { 
      "src/**.h",
      "src/**.cpp",
      "vendor/stb/**.h",
      "vendor/stb/**.cpp"
   }

   links
   {
      "GLFW",
      "glad",
      "imgui",
      "rapidyaml",
      "lua",
      "opengl32.lib",
   }

   defines
   {
      "EB_ENGINE",
	   "_CRT_SECURE_NO_WARNINGS",
      "GLFW_INCLUDE_NONE",
      "SOL_ALL_SAFETIES_ON=1"
   }

   filter "system:windows"
      systemversion "latest"  

   filter "configurations:Debug"
      defines { "EB_DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "EB_RELEASE" }
      optimize "On"

   filter {}