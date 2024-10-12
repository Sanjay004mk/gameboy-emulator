#pragma once
#include "renderer/renderer.h"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace rdr
{
	struct GPU
	{
		GPU(VkInstance instance, VkPhysicalDevice device);
		~GPU();

		VkPhysicalDevice vkPhysicalDevice = nullptr;
		VkDevice vkDevice = nullptr;
		VmaAllocator vmaAllocator = nullptr;

		uint32_t vkGraphicsQueueIndex = -1;
		VkQueue vkGraphicsQueue = nullptr;
	};

	struct RenderEngine
	{
		RenderEngine(const RendererConfiguration& config);
		~RenderEngine();

		VkInstance vkInstance = nullptr;

#if defined(RDR_DEBUG)
		VkDebugUtilsMessengerEXT vkDebugUtilsMessenger = nullptr;
#endif

		GPU* primaryDevice = nullptr;
		std::vector<GPU> workerDevices;
	};

	struct WindowCommandUnit
	{
		size_t size() const { return vkCommandBuffers.size(); }

		VkCommandBuffer& GetCommandBuffer() { return vkCommandBuffers[frameIndex]; }
		VkFence& GetFence() { return vkFences[frameIndex]; }
		VkSemaphore& GetImageSemaphore() { return vkImageAcquiredSemaphores[frameIndex]; }
		VkSemaphore& GetRenderSemaphore() { return vkRenderFinishedSemaphores[frameIndex]; }

		VkCommandPool vkCommandPool;
		std::vector<VkCommandBuffer> vkCommandBuffers;

		std::vector<VkFence> vkFences;
		std::vector<VkSemaphore> vkImageAcquiredSemaphores;
		std::vector<VkSemaphore> vkRenderFinishedSemaphores;

		uint32_t frameIndex = 0;

		WindowCommandUnit& operator++(int post)
		{
			frameIndex = (frameIndex + 1) % size();
			
			return *this;
		}
	};

	struct WindowRenderInformation
	{
		VkSurfaceKHR vkSurface;
		VkSwapchainKHR vkSwapchain;
		std::vector<std::pair<VkImage, VkImageView>> swapchainImages;
		WindowCommandUnit commandBuffer;
		uint32_t imageIndex = 0;
	};

	struct TextureImplementationInformation
	{

	};

	struct BufferImplementationInformation
	{
		VkBuffer vkBuffer = nullptr;
		VmaAllocation vmaAllocation = nullptr;
		VmaAllocationInfo vmaAllocationInfo = {};
	};
}

template <typename ostream>
ostream& operator<<(ostream& stream, VkResult result)
{
	return stream << (int)result;
}