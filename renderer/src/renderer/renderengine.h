#pragma once
#include "renderer/renderer.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace rdr
{
	struct GPU
	{
		GPU() = default;
		GPU(VkInstance instance, VkPhysicalDevice device);
		~GPU();

		GPU* Init(VkInstance instance);

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

		void WaitIdle() const;

		VkInstance vkInstance = nullptr;

#if defined(RDR_DEBUG)
		VkDebugUtilsMessengerEXT vkDebugUtilsMessenger = nullptr;
#endif
		std::vector<GPU*> allDevices;
	};

	struct CommandBuffers
	{
		CommandBuffers() = default;
		CommandBuffers(VkCommandPool commandPool, uint32_t count = 1);
		~CommandBuffers();

		void Init(VkCommandPool commandPool = nullptr, uint32_t count = 1);

		void Begin(VkCommandBufferUsageFlags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, uint32_t count = 1, uint32_t index = 0);
		void End(uint32_t count = 1, uint32_t index = 0);
		void Submit(uint32_t count = 1, uint32_t index = 0);
		void Submit(
			VkSemaphore* pWaitSemaphores, 
			uint32_t waitSemaphoresCount, 
			VkPipelineStageFlags* pWaitDstStageFlags, 
			VkSemaphore* pSignalSemaphores, 
			uint32_t signalSemaphoreCount, 
			VkFence fence = nullptr, 
			uint32_t index = 0
		);

		template <typename Integer>
		VkCommandBuffer& operator[](Integer i)  { return commandBuffers[i]; }
		VkCommandBuffer& Get(uint32_t index = 0) { return commandBuffers[index]; }

		const size_t size() const { return commandBuffers.size(); }

		static void InitCommandPool();
		static void DestroyCommandPool();

		std::vector<VkCommandBuffer> commandBuffers;
		VkCommandPool pool = nullptr;
	};

	struct WindowCommandUnit
	{
		size_t size() const { return commandBuffers.size(); }

		VkCommandBuffer& GetCommandBuffer(uint32_t index = 0) { return commandBuffers[frameIndex][index]; }
		VkCommandPool& GetCommandPool() { return vkCommandPools[frameIndex]; }
		VkFence& GetFence() { return vkFences[frameIndex]; }
		VkSemaphore& GetStartSemaphore() { return vkStartRenderSemaphores[frameIndex]; }
		VkSemaphore& GetFinishSemaphore() { return vkRenderFinishedSemaphores[frameIndex]; }

		std::vector<VkCommandPool> vkCommandPools;
		std::vector<CommandBuffers> commandBuffers;

		std::vector<VkFence> vkFences;
		std::vector<VkSemaphore> vkStartRenderSemaphores;
		std::vector<VkSemaphore> vkRenderFinishedSemaphores;

		uint32_t frameIndex = 0;

		WindowCommandUnit& operator++(int post)
		{
			frameIndex = (frameIndex + 1) % size();
			
			return *this;
		}
	};

	struct ImGuiRenderInformation
	{
		std::vector<VkFramebuffer> frameBuffers;
		VkRenderPass renderpass;
		VkDescriptorPool descriptorPool;
	};

	struct WindowRenderInformation
	{
		VkImage& GetImage() { return swapchainImages[imageIndex].first; }

		VkSurfaceKHR vkSurface;
		VkFormat surfaceFormat;
		VkSwapchainKHR vkSwapchain;
		std::vector<std::pair<VkImage, VkImageView>> swapchainImages;
		WindowCommandUnit commandBuffer, imguiCommandBuffer;
		ImGuiRenderInformation imguiInfo;

		uint32_t imageIndex = 0;
		bool mainWindow = false;
		static inline bool imguiInitialized = false;
	};

	struct TextureImplementationInformation
	{
		VkImage vkImage = nullptr;
		VkImageView vkImageView = nullptr;
		VkImageLayout vkImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VmaAllocation vmaAllocation = nullptr;
		VkDescriptorSet imguiDescriptorSet = nullptr;
		static inline VkSampler imguiSampler = nullptr;
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