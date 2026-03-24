project "Sandbox"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++23"

   targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
   objdir ("%{wks.location}/bin/int/" .. outputdir .. "/%{prj.name}")
   debugdir "%{wks.location}"

   files
   { 
      "src/**.h",
      "src/**.cpp" 
   }

   includedirs 
   {
       "%{wks.location}/Ember/src",
       "%{wks.location}/Ember/vendor",
       "%{wks.location}/Ember/vendor/glm",
   }

   links 
   {
      "Ember"
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