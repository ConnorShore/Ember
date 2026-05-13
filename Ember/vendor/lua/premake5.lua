project "lua"
    kind "StaticLib"
    language "C"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    multiprocessorcompile "On"

    files
    {
        "src/*.h",
        "src/*.c"
    }

    removefiles
    {
        "src/lua.c",
        "src/luac.c"
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

    filter "configurations:Dist"
        runtime "Release"
        staticruntime "on"
        optimize "on"