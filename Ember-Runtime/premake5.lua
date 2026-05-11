project "Ember-Runtime"
   language "C++"
   cppdialect "C++23"
   kind "ConsoleApp"

   targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
   objdir ("%{wks.location}/bin/int/" .. outputdir .. "/%{prj.name}")
   debugdir "%{wks.location}"

--    pchheader "efpch.h"
--    pchsource "src/efpch.cpp"

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
	  "%{wks.location}/Ember/vendor/rapidyaml/ext/c4core/src"
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
      -- kind "ConsoleApp" 

   filter "configurations:Release"
      defines { "EB_RELEASE" }
      optimize "On"
      -- kind "WindowedApp"
      -- entrypoint "mainCRTStartup"

   filter {}