project "Sandbox"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++23"
   targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
   objdir ("%{wks.location}/bin/int/" .. outputdir .. "/%{prj.name}")

   files
   { 
      "src/**.h",
      "src/**.cpp" 
   }

   includedirs 
   {
       "%{wks.location}/Ember/src"
   }

   links 
   {
      "Ember"
   }

   filter "configurations:Debug"
      defines { "EB_DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "EB_RELEASE" }
      optimize "On"

   filter {}