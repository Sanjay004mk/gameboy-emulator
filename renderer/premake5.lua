project "renderer"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    targetdir ("%{wks.location}/bin/" .. outputdir)
    objdir ("%{wks.location}/bin-int/" .. outputdir)

    files 
    { 
        "src/**",
        "include/**"
    }

    includedirs
    {
        "include",
        "%{IncludeDir.glfw}",
        "%{IncludeDir.VulkanSDK}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.spdlog}",
        "%{IncludeDir.vma}",
        "%{IncludeDir.stbimage}",
        "%{IncludeDir.imgui}",
        "src",
    }

    links
    {
        "glfw",
        "imgui",
        "%{Library.Vulkan}"
    }

    defines 
    {
        "RDR_EXPORT",
    }

    pchheader "renderer/rdrpch.h"
    pchsource "src/rdrpch.cpp"

    postbuildcommands "{COPYFILE} %{cfg.buildtarget.relpath} %{cfg.buildtarget.directory}/../emulator"

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        defines "RDR_DEBUG"
        runtime "Debug"
        symbols "on"

        links 
        {
            "%{Library.ShaderC_Debug}",
            "%{Library.SPIRV_Cross_Debug}",
            "%{Library.SPIRV_Tools_Debug}",
        }

    filter "configurations:Release"
        defines "RDR_RELEASE"
        runtime "Release"
        optimize "on"

        links
        {
            "%{Library.ShaderC_Release}",
            "%{Library.SPIRV_Cross_Release}"
        }