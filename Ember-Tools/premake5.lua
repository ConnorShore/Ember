project "Ember-Tools"
   kind "StaticLib"
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
      "src",
      "%{wks.location}/Ember-Tools/vendor/assimp",
      "%{wks.location}/Ember/src",
      "%{wks.location}/Ember/vendor",
      "%{wks.location}/Ember/vendor/glm",
      "%{wks.location}/Ember/vendor/lua/src",
      "%{wks.location}/Ember/vendor/sol2/include",
      "%{wks.location}/Ember/vendor/rapidyaml/src",
		"%{wks.location}/Ember/vendor/rapidyaml/ext/c4core/src"
   }

   links 
   {
      "Ember",
      "assimp"
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