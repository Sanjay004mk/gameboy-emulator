#pragma once
#include "renderer/renderer.h"

#include <vulkan/vulkan.h>

namespace rdr
{
	struct GPU
	{
		GPU(VkPhysicalDevice device);
		~GPU();

		VkPhysicalDevice mPhysicalDevice = nullptr;
		VkDevice mDevice = nullptr;

		uint32_t mGraphicsQueueIndex = -1;
		VkQueue mGraphicsQueue = nullptr;
	};

	struct RenderEngine
	{
		RenderEngine(const RendererConfiguration& config);
		~RenderEngine();

		VkInstance mVkInstance = nullptr;

#if defined(RDR_DEBUG)
		VkDebugUtilsMessengerEXT mDebugUtilsMessenger = nullptr;
#endif

		GPU* mPrimaryDevice = nullptr;
		GPU* mWorkerDevices = nullptr;
	};

	struct WindowSurfaceInformation
	{
		VkSurfaceKHR vkSurface;
		VkSwapchainKHR vkSwapchain;
		std::vector<std::pair<VkImage, VkImageView>> swapchainImages;
	};
}

template <typename ostream>
ostream& operator<<(ostream& stream, VkResult result)
{
	return stream << (int)result;
}