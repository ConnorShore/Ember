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
      "vendor/GLFW/include"
   }

   files 
   { 
      "src/**.h",
      "src/**.cpp" 
   }

   links
   {
      "GLFW",
      "opengl32.lib"
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