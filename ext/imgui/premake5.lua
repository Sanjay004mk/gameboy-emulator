project "imgui"
    kind "StaticLib"
    language "C++"

    targetdir("%{wks.location}/bin/" .. outputdir)
    objdir("%{wks.location}/bin-int/" .. outputdir)

    files
    {
        "src/imconfig.h",
        "src/imgui_draw.cpp",
        "src/imgui_impl_glfw.cpp",
        "src/imgui_impl_glfw.h",
        "src/imgui_impl_vulkan.cpp",
        "src/imgui_impl_vulkan.h",
        "src/imgui_internal.h",
        "src/imgui_tables.cpp",
        "src/imgui_widgets.cpp",
        "src/imgui.cpp",
        "src/imgui.h",
        "src/imstb_rectpack.h",
        "src/imstb_textedit.h",
        "src/imstb_truetype.h",
        "src/imgui_demo.cpp",
    }

    includedirs 
    {
        "%{IncludeDir.glfw}",
        "%{IncludeDir.VulkanSDK}"
    }

    filter "system:windows"
        systemversion "latest"
        cppdialect "C++17"
        staticruntime "Off"

    filter "system:linux"
        pic "On"
        systemversion "latest"
        cppdialect "C++17"
        staticruntime "Off"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"