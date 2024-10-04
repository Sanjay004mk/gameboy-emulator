project "emulator"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"

    targetdir ("%{wks.location}/bin/" .. outputdir)
    objdir ("%{wks.location}/bin-int/" .. outputdir)

    files "src/**"

    includedirs 
    {
        "%{wks.location}/renderer/include",
        "%{IncludeDir.glm}",
        "%{IncludeDir.spdlog}",
        "src/pch",
        "src",
    }

    pchheader "emupch.h"
    pchsource "src/pch/emupch.cpp"


    links "renderer"

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        defines "EMU_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "EMU_RELEASE"
        runtime "Release"
        optimize "on"