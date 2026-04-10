project "Ember-Forge"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++23"

   targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
   objdir ("%{wks.location}/bin/int/" .. outputdir .. "/%{prj.name}")
   debugdir "%{wks.location}"

   pchheader "efpch.h"
   pchsource "src/efpch.cpp"

   files
   { 
      "src/**.h",
      "src/**.cpp" 
   }

   includedirs 
   {
      "src",
      "vendor/ImGuizmo",
      "%{wks.location}/Ember/src",
      "%{wks.location}/Ember/vendor",
      "%{wks.location}/Ember/vendor/glm",
      "%{wks.location}/Ember/vendor/lua/src",
      "%{wks.location}/Ember/vendor/sol2/include",
      "%{wks.location}/Ember/vendor/rapidyaml/src",
		"%{wks.location}/Ember/vendor/rapidyaml/ext/c4core/src",
      "%{wks.location}/Ember-Tools/src",
      "%{wks.location}/Ember-Tools/vendor/tinygltf",
   }

   links 
   {
      "Ember",
      "Ember-Tools",
      "ImGuizmo"
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