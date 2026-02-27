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
      "src"
   }

   files 
   { 
      "src/**.h",
      "src/**.cpp" 
   }

   filter "system:windows"
      systemversion "latest"  

      defines { "EB_PLATFORM_WINDOWS" }

   filter "configurations:Debug"
      defines { "EB_DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "EB_RELEASE" }
      optimize "On"

   filter {}