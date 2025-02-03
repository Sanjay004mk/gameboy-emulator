VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["glfw"] = "%{wks.location}/ext/glfw/include"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["glm"] = "%{wks.location}/ext/glm"
IncludeDir["spdlog"] = "%{wks.location}/ext/spdlog/include"
IncludeDir["vma"] = "%{wks.location}/ext/vma/include"
IncludeDir["stbimage"] = "%{wks.location}/ext/stb_image/include"
IncludeDir["imgui"] = "%{wks.location}/ext/imgui/include"
IncludeDir["miniaudio"] = "%{wks.location}/ext/miniaudio"
IncludeDir["camelqueue"] = "%{wks.location}/ext/readerwriterqueue"

Library = {}
Library["Vulkan"] = "%{VULKAN_SDK}/Lib/vulkan-1.lib"
Library["VulkanUtils"] = "%{VULKAN_SDK}/Lib/VkLayer_utils.lib"

Library["ShaderC_Debug"] = "%{VULKAN_SDK}/Lib/shaderc_shared.lib"
Library["SPIRV_Cross_Debug"] = "%{VULKAN_SDK}/Lib/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{VULKAN_SDK}/Lib/spirv-cross-glsl.lib"
Library["SPIRV_Tools_Debug"] = "%{VULKAN_SDK}/Lib/SPIRV-Tools.lib"

Library["ShaderC_Release"] = "%{VULKAN_SDK}/Lib/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{VULKAN_SDK}/Lib/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{VULKAN_SDK}/Lib/spirv-cross-glsl.lib"