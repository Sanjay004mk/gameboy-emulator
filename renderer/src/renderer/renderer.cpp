#include <renderer/rdrpch.h>

#include "renderer/rdr.h"
#include "renderer/time.h"

#include "renderengine.h"

#include <glfw/glfw3.h>

#include <glm/glm.hpp>

#include <stb_image.h>

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

namespace utils
{
	static VkFormat to_vk_format(rdr::TextureFormat format)
	{
		if (format.isDepth)
		{
			if (format.channels == 2)
				return VK_FORMAT_D24_UNORM_S8_UINT;
			else
				return VK_FORMAT_D16_UNORM;
		}

		static constexpr uint32_t channelStart8Bit[] = {
			0,
			9,
			16,
			23,
			37
		};

		// TODO add bits per channel
		uint32_t vkformat = 0;
		vkformat += channelStart8Bit[format.channels];

		uint32_t offset = 0;

		if (format.type & rdr::eDataType::TInt)
			offset += 4;

		if (format.type & rdr::eDataType::Signed)
			offset++;

		if (format.reverse)
			offset += 7;

		return (VkFormat)(vkformat + offset);
	}

	static void transition_vk_image_layout(VkCommandBuffer cb, VkImage image, VkImageLayout from, VkImageLayout to)
	{
		VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.image = image;
		barrier.oldLayout = from;
		barrier.newLayout = to;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		VkPipelineStageFlags sourceStage = 0;
		VkPipelineStageFlags destinationStage = 0;

		if (from == VK_IMAGE_LAYOUT_UNDEFINED && to == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_NONE;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (from == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && to == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (from == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && to == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (from == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && to == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (from == VK_IMAGE_LAYOUT_UNDEFINED && to == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_NONE;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (from == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && to == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else {
			RDR_LOG_ERROR("Vulkan: Unhandled case of image layout transition!");
			return;
		}

		vkCmdPipelineBarrier(
			cb,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}

	static void blit_image(VkCommandBuffer cb, VkImage src, VkImage dst, const rdr::TextureBlitInformation& blitInfo)
	{
		VkImageBlit imageBlit{};
		imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.dstSubresource.baseArrayLayer = 0;
		imageBlit.dstSubresource.layerCount = 1;
		imageBlit.dstSubresource.mipLevel = 0;
		imageBlit.srcSubresource = imageBlit.dstSubresource;
		imageBlit.dstOffsets[0] = { blitInfo.dstMin.x, blitInfo.dstMin.y, 0 };
		imageBlit.srcOffsets[0] = { blitInfo.srcMin.x, blitInfo.srcMin.y, 0 };
		imageBlit.dstOffsets[1] = { blitInfo.dstMax.x, blitInfo.dstMax.y, 1 };
		imageBlit.srcOffsets[1] = { blitInfo.srcMax.x, blitInfo.srcMax.y, 1 };

		vkCmdBlitImage(
			cb,
			src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &imageBlit,
			(VkFilter)blitInfo.filter
		);
	}
}


namespace rdr
{
	thread_local static GPU* primaryGPU = nullptr;
	thread_local static VkCommandPool gPool = nullptr;
	thread_local static Window* renderWindow = nullptr;

	Renderer::Renderer(const RendererConfiguration& config)
		: mConfig(config)
	{
		// GLFW initialization
		{
			RDR_LOG_TRACE("Initializing GLFW");
			int result = glfwInit();
			RDR_ASSERT_MSG_BREAK(result == GLFW_TRUE, "Failed to initialize GLFW");

			glfwSetErrorCallback([](int error, const char* description) {
				RDR_LOG_ERROR("GLFW: Error {}: {}", error, description);
				});

			result = glfwVulkanSupported();

			RDR_ASSERT_MSG_BREAK(result == GLFW_TRUE, "Vulkan Not Supported");
		}

		// RenderEngine (Vulkan) Initialization
		{
			mRenderEngine = new RenderEngine(mConfig);
		}
	}

	Renderer::~Renderer()
	{
		RDR_LOG_TRACE("Shutting down Renderer");

		delete mRenderEngine;

		glfwTerminate();
	}

	void Renderer::Init(int argc, const char** argv, std::string_view applicationName)
	{
		Logger::Init(applicationName);
		mRenderer = new Renderer({ CommandLineArgumentsList(argc, argv), std::string(applicationName) });
	}

	void Renderer::Shutdown()
	{
		for (auto& window : mRenderer->mWindows)
			delete window;

		delete mRenderer;
		Logger::Shutdown();
	}

	void Renderer::PollEvents()
	{
		glfwPollEvents();
	}

	void Renderer::WaitEvents()
	{
		glfwWaitEvents();
	}

	Window* Renderer::InstantiateWindow(const WindowConfiguration& windowConfig)
	{
		return mRenderer->mWindows.emplace_back(new Window(windowConfig));
	}

	void Renderer::FreeWindow(Window* window)
	{
		auto& windows = mRenderer->mWindows;
		auto it = std::find(windows.begin(), windows.end(), window);

		if (it != windows.end())
		{
			delete window;
			windows.erase(it);
		}
	}

	void Renderer::BeginFrame(Window* window, const glm::vec4& clearColor)
	{
		renderWindow = window;
		renderWindow->BeginFrame(clearColor);
	}

	void Renderer::EndFrame()
	{
		renderWindow->EndFrame();
		renderWindow = nullptr;
	}

	void Renderer::Blit(Texture* srcTexture, Texture* dstTexture, const TextureBlitInformation& blitInfo)
	{
		utils::blit_image(
			renderWindow->mConfig.renderInfo->commandBuffer.GetCommandBuffer(), 
			srcTexture->GetConfig().impl->vkImage,
			dstTexture->GetConfig().impl->vkImage,
			blitInfo
		);
	}

	void Renderer::BlitToWindow(Texture* texture, const TextureBlitInformation& blitInfo)
	{
		if (renderWindow->mConfig.minimized)
			return;

		utils::blit_image(
			renderWindow->mConfig.renderInfo->commandBuffer.GetCommandBuffer(),
			texture->GetConfig().impl->vkImage,
			renderWindow->mConfig.renderInfo->GetImage(),
			blitInfo
		);
	}

	const GPUHandle Renderer::GetPrimaryGPU()
	{
		return primaryGPU;
	}

	void Renderer::SetPrimaryGPU(GPUHandle gpu)
	{
		primaryGPU = gpu;
	}

	const std::vector<GPUHandle>& Renderer::GetAllGPUs()
	{
		return Get()->mRenderEngine->allDevices;
	}

	void Renderer::InitGPU(GPUHandle gpu)
	{
		gpu->Init(Get()->mRenderEngine->vkInstance);
	}

	bool Renderer::IsActive(GPUHandle gpu)
	{
		return gpu->vkDevice;
	}

	RenderEngine::RenderEngine(const RendererConfiguration& config)
	{
		VkResult err = VK_SUCCESS;
		// Instance creation
		{
			uint32_t extensionCount = 0;
			const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

			RDR_ASSERT_MSG_BREAK(extensionCount != 0, "Vulkan rendering not supported");

			VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
			appInfo.apiVersion = VK_API_VERSION_1_3;
			appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
			appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
			appInfo.pApplicationName = config.applicationName.c_str();
			appInfo.pEngineName = "renderer";

			VkInstanceCreateInfo instanceCreateInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
			instanceCreateInfo.pApplicationInfo = &appInfo;
			instanceCreateInfo.enabledExtensionCount = extensionCount;
			instanceCreateInfo.ppEnabledExtensionNames = extensions;

#if defined(RDR_DEBUG)
			const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
			
			instanceCreateInfo.enabledLayerCount = 1;
			instanceCreateInfo.ppEnabledLayerNames = layers;

			std::vector<const char*> extensions_ext(extensions, extensions + extensionCount);
			extensions_ext.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

			instanceCreateInfo.enabledExtensionCount += 1;
			instanceCreateInfo.ppEnabledExtensionNames = extensions_ext.data();
#endif

			err = vkCreateInstance(&instanceCreateInfo, nullptr, &vkInstance);
			RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan {}: Failed to create VkInstance", err);

#if defined(RDR_DEBUG)

			auto FNvkCreateDebugUtilsMessengerExt = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT");
			RDR_ASSERT_NO_MSG_BREAK(FNvkCreateDebugUtilsMessengerExt);

			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = 
			{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
			debugCreateInfo.messageSeverity = 
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | 
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | 
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
			debugCreateInfo.messageType = 
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | 
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
			debugCreateInfo.pfnUserCallback = [](
				VkDebugUtilsMessageSeverityFlagBitsEXT          messageSeverity,
				VkDebugUtilsMessageTypeFlagsEXT                 messageTypes,
				const VkDebugUtilsMessengerCallbackDataEXT*		pCallbackData,
				void*											pUserData) -> VkBool32
				{
					if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
						RDR_LOG_ERROR("Vulkan DEBUG_ERROR: {0}", pCallbackData->pMessage);
					else
						RDR_LOG_TRACE("Vulkan DEBUG_TRACE: {0}", pCallbackData->pMessage);

					return VK_FALSE;
				};

			err = FNvkCreateDebugUtilsMessengerExt(vkInstance, &debugCreateInfo, nullptr, &vkDebugUtilsMessenger);
			RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan {}: Failed to create Debug messenger");
#endif

			RDR_LOG_TRACE("Created Vulkan Instance");
		}

		// Choosing Primary GPU
		{
			uint32_t count = 0;
			vkEnumeratePhysicalDevices(vkInstance, &count, nullptr);
			RDR_ASSERT_NO_MSG_BREAK(count != 0);

			std::vector<VkPhysicalDevice> devices(count);
			vkEnumeratePhysicalDevices(vkInstance, &count, devices.data());

			allDevices.resize(count);
			bool primarySet = false;

			VkPhysicalDeviceProperties properties{};
			for (uint32_t i = 0; i < count; i++)
			{
				allDevices[i] = new GPU();
				allDevices[i]->vkPhysicalDevice = devices[i];

				vkGetPhysicalDeviceProperties(devices[i], &properties);
				if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && !primarySet)
				{
					RDR_LOG_TRACE("Vulkan: Using {} as primary GPU", properties.deviceName);
					primaryGPU = allDevices[i]->Init(vkInstance);
					primarySet = true;
				}
			}

			// TODO better device selection
			if (!primaryGPU) // default to first device
				primaryGPU = allDevices[0]->Init(vkInstance);
		}

		// Initialize global Command pool
		CommandBuffers::InitCommandPool();

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;
		//ImGui::DockSpace(ImGui::GetID("MyDockspace"));

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsLight();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
	}

	RenderEngine::~RenderEngine()
	{
		ImGui::DestroyContext();

		if (TextureImplementationInformation::imguiSampler)
			vkDestroySampler(primaryGPU->vkDevice, TextureImplementationInformation::imguiSampler, nullptr);

		CommandBuffers::DestroyCommandPool();

		delete primaryGPU;

#if defined(RDR_DEBUG)
		auto FNvkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugUtilsMessengerEXT");
		RDR_ASSERT_NO_MSG_BREAK(FNvkDestroyDebugUtilsMessengerEXT);

		FNvkDestroyDebugUtilsMessengerEXT(vkInstance, vkDebugUtilsMessenger, nullptr);
#endif

		vkDestroyInstance(vkInstance, nullptr);
		RDR_LOG_TRACE("Destroyed Vulkan instance");
	}

	void RenderEngine::WaitIdle() const
	{

		for (auto& device : allDevices)
			if (device)
				vkDeviceWaitIdle(device->vkDevice);
	}

	GPU::GPU(VkInstance vkInstance, VkPhysicalDevice device)
		: vkPhysicalDevice(device)
	{
		Init(vkInstance);
	}

	GPU* GPU::Init(VkInstance vkInstance)
	{
		VkResult err{};
		// Graphics queue
		{
			uint32_t count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &count, nullptr);
			std::vector<VkQueueFamilyProperties> properties(count);
			vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &count, properties.data());

			for (uint32_t queueIndex = 0; queueIndex < count; queueIndex++)
			{
				if (properties[queueIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					vkGraphicsQueueIndex = queueIndex;
					break;
				}
			}

			RDR_ASSERT_MSG_BREAK(vkGraphicsQueueIndex != -1, "Vulkan: Failed to find suitable graphics queue");
		}

		// Logical Device
		{

			VkPhysicalDeviceFeatures deviceFeatures{};
			deviceFeatures.fillModeNonSolid = VK_TRUE;
			deviceFeatures.shaderStorageImageArrayDynamicIndexing = VK_TRUE;

			std::array<const char*, 1> deviceExtensions = {
				"VK_KHR_swapchain",
			};

			float priorities[] = { 1.f };

			VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.queueFamilyIndex = vkGraphicsQueueIndex;
			queueCreateInfo.pQueuePriorities = priorities;

			VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
			deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
			deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
			deviceCreateInfo.queueCreateInfoCount = 1;
			deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
			
			err = vkCreateDevice(vkPhysicalDevice, &deviceCreateInfo, nullptr, &vkDevice);
			RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan {}: Failed to create logical device", err);

			vkGetDeviceQueue(vkDevice, vkGraphicsQueueIndex, 0, &vkGraphicsQueue);
		}

		RDR_LOG_TRACE("Vulkan: Created logical device");

		// VMA initialization
		{
			VmaAllocatorCreateInfo allocatorCreateInfo{};
			allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
			allocatorCreateInfo.device = vkDevice;
			allocatorCreateInfo.physicalDevice = vkPhysicalDevice;
			allocatorCreateInfo.instance = vkInstance;

			err = vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator);
			RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan {}: Failed to create vma allocator", err);
		}

		return this;
	}

	GPU::~GPU()
	{
		if (vkDevice)
		{
			vmaDestroyAllocator(vmaAllocator);
			vkDestroyDevice(vkDevice, nullptr);
		}
	}

	void CommandBuffers::InitCommandPool()
	{
		VkCommandPoolCreateInfo info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		info.queueFamilyIndex = Renderer::GetPrimaryGPU()->vkGraphicsQueueIndex;

		VkResult err = vkCreateCommandPool(Renderer::GetPrimaryGPU()->vkDevice, &info, nullptr, &gPool);
		RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan {}: Failed to create command pool", err);
	}

	void CommandBuffers::DestroyCommandPool()
	{
		vkDestroyCommandPool(Renderer::GetPrimaryGPU()->vkDevice, gPool, nullptr);
	}

	CommandBuffers::CommandBuffers(VkCommandPool commandPool, uint32_t count)
	{
		Init(commandPool, count);
	}

	void CommandBuffers::Init(VkCommandPool commandPool, uint32_t count)
	{
		pool = commandPool == nullptr ? gPool : commandPool;
		commandBuffers.resize(count);

		VkCommandBufferAllocateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandBufferCount = count;
		info.commandPool = pool;
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		VkResult err = vkAllocateCommandBuffers(Renderer::GetPrimaryGPU()->vkDevice, &info, commandBuffers.data());
		RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan {}: Failed to create command buffer", err);
	}

	CommandBuffers::~CommandBuffers()
	{
		vkFreeCommandBuffers(Renderer::GetPrimaryGPU()->vkDevice, pool, (uint32_t)commandBuffers.size(), commandBuffers.data());
	}

	void CommandBuffers::Begin(VkCommandBufferUsageFlags flags, uint32_t count, uint32_t index)
	{
		vkResetCommandPool(Renderer::GetPrimaryGPU()->vkDevice, pool, 0);

		VkCommandBufferBeginInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.flags = flags;

		for (uint32_t i = index; i < index + count; i++)
			vkBeginCommandBuffer(commandBuffers[i], &info);
	}

	void CommandBuffers::End(uint32_t count, uint32_t index)
	{
		for (uint32_t i = index; i < index + count; i++)
			vkEndCommandBuffer(commandBuffers[i]);
	}

	void CommandBuffers::Submit(uint32_t count, uint32_t index)
	{
		VkSubmitInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.pCommandBuffers = &commandBuffers[index];
		info.commandBufferCount = count;
		VkResult err = vkQueueSubmit(Renderer::GetPrimaryGPU()->vkGraphicsQueue, 1, &info, nullptr);
		RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan {}: Failed to submit command buffer", err);

		vkDeviceWaitIdle(Renderer::GetPrimaryGPU()->vkDevice);
	}

	void CommandBuffers::Submit(VkSemaphore* pWaitSemaphores, uint32_t waitSemaphoresCount, VkPipelineStageFlags* pWaitDstStageFlags, VkSemaphore* pSignalSemaphores, uint32_t signalSemaphoreCount, VkFence fence, uint32_t index)
	{
		VkSubmitInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.pCommandBuffers = &commandBuffers[index];
		info.commandBufferCount = 1;

		info.pSignalSemaphores = pSignalSemaphores;
		info.pWaitDstStageMask = pWaitDstStageFlags;
		info.pWaitSemaphores = pWaitSemaphores;
		info.signalSemaphoreCount = signalSemaphoreCount;
		info.waitSemaphoreCount = waitSemaphoresCount;

		VkResult err = vkQueueSubmit(Renderer::GetPrimaryGPU()->vkGraphicsQueue, 1, &info, nullptr);
		RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan {}: Failed to submit command buffer", err);
	}

	void Window::SetupSurface()
	{
		mConfig.renderInfo = new WindowRenderInformation();
		VkSurfaceKHR& surface = mConfig.renderInfo->vkSurface;
		VkResult err = glfwCreateWindowSurface(Renderer::Get()->mRenderEngine->vkInstance, mGlfwWindow, nullptr, &surface);
		RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan|GLFW {}: Failed to create window surface", err);

		VkBool32 supported = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(mConfig.gpuDevice->vkPhysicalDevice, mConfig.gpuDevice->vkGraphicsQueueIndex, surface, &supported);
		RDR_ASSERT_MSG_BREAK(supported == VK_TRUE, "Vulkan: Device doesn't support required window format");
	}

	void Window::SetupSwapchain()
	{
		VkPhysicalDevice& physicalDevice = mConfig.gpuDevice->vkPhysicalDevice;
		VkDevice& device = mConfig.gpuDevice->vkDevice;
		VkSurfaceKHR& surface = mConfig.renderInfo->vkSurface;
		VkSwapchainKHR& swapchain = mConfig.renderInfo->vkSwapchain;
		VkResult err{};

		VkSurfaceFormatKHR useFormat{};
		{
			bool surfaceFormatSet = false;

			std::array<VkFormat, 5> formats = { 
				VK_FORMAT_B8G8R8A8_UNORM, 
				VK_FORMAT_B8G8R8A8_SRGB,
				VK_FORMAT_R8G8B8A8_SNORM, 
				VK_FORMAT_B8G8R8_UNORM, 
				VK_FORMAT_R8G8B8_SNORM 
			};
			VkColorSpaceKHR requiredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

			uint32_t count = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, nullptr);
			RDR_ASSERT_NO_MSG_BREAK(count != 0);
			std::vector<VkSurfaceFormatKHR> surfaceFormats(count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, surfaceFormats.data());

			for (auto& requiredFormat : formats)
			{
				for (auto& availableFormat : surfaceFormats)
				{
					if (availableFormat.format == requiredFormat && availableFormat.colorSpace == requiredColorSpace)
					{
						useFormat = availableFormat;
						surfaceFormatSet = true;
						break;
					}
				}
				if (surfaceFormatSet)
					break;
			}

			if (!surfaceFormatSet)
			{
				if (surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
					useFormat = { formats[0], requiredColorSpace };
				else
					useFormat = surfaceFormats[0];
			}

		}

		VkPresentModeKHR usePresentMode{};
		{
			bool presentModeSet = false;

			std::array<VkPresentModeKHR, 5> modes = { 
				(VkPresentModeKHR)mConfig.presentMode,
				(VkPresentModeKHR)PresentMode::NoVSync,
				(VkPresentModeKHR)PresentMode::TripleBuffer, 
				(VkPresentModeKHR)PresentMode::VSync, 
				(VkPresentModeKHR)PresentMode::VSyncRelaxed
			};

			uint32_t count = 0;
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, nullptr);
			RDR_ASSERT_NO_MSG_BREAK(count != 0);
			std::vector<VkPresentModeKHR> presentModesAvailable(count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, presentModesAvailable.data());

			for (auto& required : modes)
			{
				for (auto& available : presentModesAvailable)
				{
					if (required == available)
					{
						usePresentMode = required;
						presentModeSet = true;
						break;
					}
				}
				if (presentModeSet)
					break;
			}

			if (!presentModeSet)
				usePresentMode = VK_PRESENT_MODE_FIFO_KHR;
		}

		VkSurfaceCapabilitiesKHR surfaceCapabilities{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

		VkExtent2D extent = surfaceCapabilities.currentExtent;
		extent = {
			glm::clamp(extent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
			glm::clamp(extent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)
		};

		VkSwapchainCreateInfoKHR swapchainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		swapchainCreateInfo.imageFormat = useFormat.format;
		swapchainCreateInfo.imageColorSpace = useFormat.colorSpace;
		swapchainCreateInfo.presentMode = usePresentMode;
		swapchainCreateInfo.minImageCount = glm::min(surfaceCapabilities.maxImageCount, 2u);
		swapchainCreateInfo.imageExtent = extent;

		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // TODO add application transparency
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		swapchainCreateInfo.oldSwapchain = swapchain;
		swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
		swapchainCreateInfo.surface = surface;

		err = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);
		RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan {}: Failed to create swapchain", err);

		mConfig.renderInfo->surfaceFormat = useFormat.format;

		uint32_t imageCount = 0;
		vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
		std::vector<VkImage> images(imageCount);
		vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data());

		VkImageViewCreateInfo imageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		imageViewCreateInfo.format = useFormat.format;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.subresourceRange.levelCount = 1;

		mConfig.renderInfo->swapchainImages.resize(imageCount);
		uint32_t index = 0;
		for (auto& [image, view] : mConfig.renderInfo->swapchainImages)
		{
			image = imageViewCreateInfo.image = images[index++];

			err = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &view);
			RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan {}: Failed to create image view", err);
		}		
	}

	void Window::SetupCommandUnit()
	{
		size_t imageCount = mConfig.renderInfo->swapchainImages.size();
		RDR_ASSERT_MSG_BREAK(imageCount != 0, "Vulkan: Creating command buffers before swapchain is created");

		VkResult err{};
		VkDevice vkDevice = mConfig.gpuDevice->vkDevice;

		WindowCommandUnit* commandBuffers[] = { &mConfig.renderInfo->commandBuffer, &mConfig.renderInfo->imguiCommandBuffer };
		for (auto& cb : commandBuffers)
		{
			cb->vkCommandPools.resize(imageCount);
			cb->commandBuffers.resize(imageCount);
			cb->vkFences.resize(imageCount);
			cb->vkStartRenderSemaphores.resize(imageCount);
			cb->vkRenderFinishedSemaphores.resize(imageCount);

			VkCommandPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
			poolCreateInfo.queueFamilyIndex = mConfig.gpuDevice->vkGraphicsQueueIndex;

			for (auto& pool : cb->vkCommandPools)
			{
				err = vkCreateCommandPool(vkDevice, &poolCreateInfo, nullptr, &pool);
				RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan {}: Failed to create command pool", err);
			}

			for (uint32_t i = 0; i < imageCount; i++)
			{
				cb->commandBuffers[i].Init(cb->vkCommandPools[i], (uint32_t)imageCount); // TODO add better no. of command buffers to allocate by default
			}

			VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
			fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

			for (size_t i = 0; i < imageCount; i++)
			{
				err = vkCreateFence(vkDevice, &fenceCreateInfo, nullptr, &cb->vkFences[i]);
				RDR_ASSERT_NO_MSG_BREAK(err == VK_SUCCESS);

				err = vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, nullptr, &cb->vkStartRenderSemaphores[i]);
				RDR_ASSERT_NO_MSG_BREAK(err == VK_SUCCESS);

				err = vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, nullptr, &cb->vkRenderFinishedSemaphores[i]);
				RDR_ASSERT_NO_MSG_BREAK(err == VK_SUCCESS);
			}
		}
	}

	void Window::SetupImGui()
	{
		if (WindowRenderInformation::imguiInitialized)
			return;

		WindowRenderInformation::imguiInitialized = mConfig.renderInfo->mainWindow = true;

		size_t imageCount = mConfig.renderInfo->swapchainImages.size();
		VkResult err;

		// imgui Descriptor pool creation
		{
			VkDescriptorPoolSize poolSizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
			};

			VkDescriptorPoolCreateInfo poolCI{};
			poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			poolCI.maxSets = 1000 * (uint32_t)(sizeof(poolSizes) / sizeof(VkDescriptorPoolSize));
			poolCI.poolSizeCount = (uint32_t)(sizeof(poolSizes) / sizeof(VkDescriptorPoolSize));
			poolCI.pPoolSizes = poolSizes;

			err = vkCreateDescriptorPool(primaryGPU->vkDevice, &poolCI, nullptr, &mConfig.renderInfo->imguiInfo.descriptorPool);
			RDR_ASSERT_MSG(err == VK_SUCCESS, "Vulkan {}: Failed to create descriptor pool!", err);
		}

		// imgui render pass creation
		{

			VkAttachmentDescription attachment = {};
			attachment.format = mConfig.renderInfo->surfaceFormat;
			attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference color_attachment = {};
			color_attachment.attachment = 0;
			color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &color_attachment;

			VkSubpassDependency dependency = {};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			VkRenderPassCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			info.attachmentCount = 1;
			info.pAttachments = &attachment;
			info.subpassCount = 1;
			info.pSubpasses = &subpass;
			info.dependencyCount = 1;
			info.pDependencies = &dependency;
			err = vkCreateRenderPass(primaryGPU->vkDevice, &info, nullptr, &mConfig.renderInfo->imguiInfo.renderpass);
			RDR_ASSERT_MSG(err == VK_SUCCESS, "Vulkan {}: Failed to create render pass!", err);
		}

		// imgui initialization
		{
			ImGui_ImplGlfw_InitForVulkan(mGlfwWindow, true);

			ImGui_ImplVulkan_InitInfo info{};
			info.Instance = Renderer::mRenderer->mRenderEngine->vkInstance;
			info.PhysicalDevice = primaryGPU->vkPhysicalDevice;
			info.Device = primaryGPU->vkDevice;
			info.QueueFamily = primaryGPU->vkGraphicsQueueIndex;
			info.Queue = primaryGPU->vkGraphicsQueue;
			info.PipelineCache = VK_NULL_HANDLE;
			info.DescriptorPool = mConfig.renderInfo->imguiInfo.descriptorPool;
			info.Subpass = 0;
			info.MinImageCount = (uint32_t)imageCount;
			info.ImageCount = info.MinImageCount;
			info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
			info.Allocator = nullptr;
			info.CheckVkResultFn = [](VkResult err)
				{
					RDR_ASSERT_MSG(err == VK_SUCCESS, "Vulkan: ImGui error: {}", err);
				};

			ImGui_ImplVulkan_Init(&info, mConfig.renderInfo->imguiInfo.renderpass);
		}

		// setup fonts
		{
			CommandBuffers cb;
			cb.Init();
			cb.Begin();

			ImGui_ImplVulkan_CreateFontsTexture(cb.Get());

			cb.End();
			cb.Submit();
			vkDeviceWaitIdle(primaryGPU->vkDevice);

			ImGui_ImplVulkan_DestroyFontUploadObjects();
		}
	}

	void Window::SetupFramebuffer()
	{
		size_t imageCount = mConfig.renderInfo->swapchainImages.size();

		/* create framebuffers */
		uint32_t width = mConfig.size.x;
		uint32_t height = mConfig.size.y;

		std::vector<std::vector<VkImageView>> attachments;

		for (uint32_t i = 0; i < imageCount; i++)
			attachments.push_back({ mConfig.renderInfo->swapchainImages[i].second });

		auto& fb = mConfig.renderInfo->imguiInfo.frameBuffers;
		fb.resize(imageCount);
		for (size_t i = 0; i < fb.size(); i++)
		{
			VkFramebufferCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			info.attachmentCount = (uint32_t)attachments[i].size();
			info.height = height;
			info.width = width;
			info.layers = 1;
			info.pAttachments = attachments[i].data();
			info.renderPass = mConfig.renderInfo->imguiInfo.renderpass;

			VkResult err = vkCreateFramebuffer(primaryGPU->vkDevice, &info, nullptr, &fb[i]);
			RDR_ASSERT_MSG(err == VK_SUCCESS, "Vulkan {}: Failed to create framebuffer!", err);
		}
	}

	void Window::CleanupSurface()
	{
		VkSurfaceKHR& surface = mConfig.renderInfo->vkSurface;
		vkDestroySurfaceKHR(Renderer::Get()->mRenderEngine->vkInstance, surface, nullptr);

		delete mConfig.renderInfo;
	}

	void Window::CleanupSwapchain()
	{
		for (auto& [image, view] : mConfig.renderInfo->swapchainImages)
			vkDestroyImageView(mConfig.gpuDevice->vkDevice, view, nullptr);
		
		vkDestroySwapchainKHR(mConfig.gpuDevice->vkDevice, mConfig.renderInfo->vkSwapchain, nullptr);

		mConfig.renderInfo->vkSwapchain = nullptr;
	}

	void Window::CleanupCommandUnit()
	{
		VkDevice vkDevice = mConfig.gpuDevice->vkDevice;

		vkDeviceWaitIdle(vkDevice);

		WindowCommandUnit* commandBuffers[] = { &mConfig.renderInfo->commandBuffer, &mConfig.renderInfo->imguiCommandBuffer };

		for (auto& cb : commandBuffers)
		{
			cb->commandBuffers.clear();

			for (auto& pool : cb->vkCommandPools)
				vkDestroyCommandPool(vkDevice, pool, nullptr);
			cb->vkCommandPools.clear();

			for (auto& fence : cb->vkFences)
				vkDestroyFence(vkDevice, fence, nullptr);
			cb->vkFences.clear();

			for (auto& semaphore : cb->vkStartRenderSemaphores)
				vkDestroySemaphore(vkDevice, semaphore, nullptr);
			cb->vkStartRenderSemaphores.clear();

			for (auto& semaphore : cb->vkRenderFinishedSemaphores)
				vkDestroySemaphore(vkDevice, semaphore, nullptr);
			cb->vkRenderFinishedSemaphores.clear();
		}
	}

	void Window::CleanupImGui()
	{
		if (!mConfig.renderInfo->mainWindow)
			return;

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();

		vkDestroyDescriptorPool(primaryGPU->vkDevice, mConfig.renderInfo->imguiInfo.descriptorPool, nullptr);

		vkDestroyRenderPass(primaryGPU->vkDevice, mConfig.renderInfo->imguiInfo.renderpass, nullptr);
	}

	void Window::CleanupFramebuffer()
	{

		for (auto& fb : mConfig.renderInfo->imguiInfo.frameBuffers)
			vkDestroyFramebuffer(primaryGPU->vkDevice, fb, nullptr);
	}

	void Window::ResetSwapchain()
	{
		if (mConfig.minimized)
			return;

		VkSwapchainKHR oldSwapchain = mConfig.renderInfo->vkSwapchain;
		size_t oldImageCount = mConfig.renderInfo->swapchainImages.size();

		for (auto& [image, view] : mConfig.renderInfo->swapchainImages)
			vkDestroyImageView(mConfig.gpuDevice->vkDevice, view, nullptr);

		SetupSwapchain();

		if (mConfig.renderInfo->swapchainImages.size() != oldImageCount)
		{
			CleanupCommandUnit();
			SetupCommandUnit();
		}
		CleanupFramebuffer();
		SetupFramebuffer();

		vkDestroySwapchainKHR(mConfig.gpuDevice->vkDevice, oldSwapchain, nullptr);
	}

	void Window::BeginFrame(const glm::vec4& clearColor)
	{
		if (mConfig.minimized)
			return;

		WindowCommandUnit& cb = mConfig.renderInfo->commandBuffer;
		uint32_t& index = mConfig.renderInfo->imageIndex;

		vkWaitForFences(mConfig.gpuDevice->vkDevice, 1, &cb.GetFence(), VK_TRUE, UINT64_MAX);

		vkAcquireNextImageKHR(
			mConfig.gpuDevice->vkDevice, 
			mConfig.renderInfo->vkSwapchain, 
			UINT64_MAX, 
			cb.GetStartSemaphore(),
			VK_NULL_HANDLE, 
			&index
		);

		vkResetFences(mConfig.gpuDevice->vkDevice, 1, &cb.GetFence());
		vkResetCommandPool(mConfig.gpuDevice->vkDevice, cb.GetCommandPool(), 0);

		VkImage& image = mConfig.renderInfo->swapchainImages[index].first;
		VkImageView& view = mConfig.renderInfo->swapchainImages[index].second;

		VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(cb.GetCommandBuffer(), &beginInfo);

		VkClearColorValue vkclearColor = {
			clearColor.r,
			clearColor.g,
			clearColor.b,
			clearColor.a
		};

		VkImageSubresourceRange subresourceRange{};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.layerCount = 1;
		subresourceRange.levelCount = 1;

		VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_NONE;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.subresourceRange = subresourceRange;
		barrier.image = image;

		vkCmdPipelineBarrier(
			cb.GetCommandBuffer(),
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		vkCmdClearColorImage(
			cb.GetCommandBuffer(),
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			&vkclearColor,
			1,
			&subresourceRange
		);

		if (mConfig.renderInfo->mainWindow)
		{
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		}
	}

	void Window::EndFrame()
	{
		if (mConfig.minimized)
			return;

		WindowCommandUnit& cb = mConfig.renderInfo->commandBuffer;
		WindowCommandUnit& imguicb = mConfig.renderInfo->imguiCommandBuffer;
		uint32_t& index = mConfig.renderInfo->imageIndex;

		VkImage& image = mConfig.renderInfo->swapchainImages[index].first;

		vkEndCommandBuffer(cb.GetCommandBuffer());

		VkPipelineStageFlags dstStageMask[] = { VK_PIPELINE_STAGE_TRANSFER_BIT };

		VkSubmitInfo info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		info.commandBufferCount = 1;
		info.pCommandBuffers = &cb.GetCommandBuffer();
		info.signalSemaphoreCount = 1;
		info.pSignalSemaphores = &cb.GetFinishSemaphore();
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &cb.GetStartSemaphore();
		info.pWaitDstStageMask = dstStageMask;

		VkFence fence = mConfig.renderInfo->mainWindow ? VK_NULL_HANDLE : cb.GetFence();
		VkResult err = vkQueueSubmit(mConfig.gpuDevice->vkGraphicsQueue, 1, &info, fence);

		// render imgui
		if (mConfig.renderInfo->mainWindow)
		{
			ImGuiIO& io = ImGui::GetIO();
			io.DisplaySize = ImVec2((float)mConfig.size.x, (float)mConfig.size.y);

			ImGui::Render();
			ImDrawData* main_draw_data = ImGui::GetDrawData();

			vkResetCommandPool(mConfig.gpuDevice->vkDevice, imguicb.GetCommandPool(), 0);

			VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			vkBeginCommandBuffer(imguicb.GetCommandBuffer(), &beginInfo);

			VkRenderPassBeginInfo rpinfo{};
			rpinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			rpinfo.framebuffer = mConfig.renderInfo->imguiInfo.frameBuffers[index];
			rpinfo.renderPass = mConfig.renderInfo->imguiInfo.renderpass;
			rpinfo.renderArea.extent = { mConfig.size.x , mConfig.size.y };
			rpinfo.clearValueCount = 0;

			vkCmdBeginRenderPass(imguicb.GetCommandBuffer(), &rpinfo, VK_SUBPASS_CONTENTS_INLINE);

			ImGui_ImplVulkan_RenderDrawData(main_draw_data, imguicb.GetCommandBuffer());

			vkCmdEndRenderPass(imguicb.GetCommandBuffer());

			VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			vkEndCommandBuffer(imguicb.GetCommandBuffer());

			VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &imguicb.GetCommandBuffer();
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &imguicb.GetFinishSemaphore();
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &cb.GetFinishSemaphore();
			submitInfo.pWaitDstStageMask = dstStageMask;

			VkResult err = vkQueueSubmit(mConfig.gpuDevice->vkGraphicsQueue, 1, &submitInfo, cb.GetFence());

			// Update and Render additional Platform Windows
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}

		}

		vkQueueWaitIdle(mConfig.gpuDevice->vkGraphicsQueue);

		VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &mConfig.renderInfo->vkSwapchain;
		presentInfo.pImageIndices = &index;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = mConfig.renderInfo->mainWindow ? &imguicb.GetFinishSemaphore() : &cb.GetFinishSemaphore();

		vkQueuePresentKHR(mConfig.gpuDevice->vkGraphicsQueue, &presentInfo);

		cb++;
		imguicb++;
	}

	Buffer::Buffer(const BufferConfiguration& config, const void* data)
		: mConfig(config)
	{
		VmaAllocationCreateInfo allocationCreateInfo{};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };

		mConfig.impl = new BufferImplementationInformation();
		BufferImplementationInformation& impl = *mConfig.impl;

		createInfo.size = config.size;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (mConfig.enableCopy)
			createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		switch (config.type)
		{
		case BufferType::StagingBuffer:
			mConfig.cpuVisible = true;
			createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			if (mConfig.enableCopy)
				createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

			allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
			break;

		case BufferType::GPUBuffer:
			mConfig.cpuVisible = false;
			createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
			break;

		case BufferType::UniformBuffer:
			mConfig.cpuVisible = true;
			createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
			break;

		case BufferType::VertexBuffer:
			mConfig.cpuVisible = false;
			createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
			break;

		case BufferType::IndexBuffer:
			mConfig.cpuVisible = false;
			createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
			break;
		}

		if (mConfig.persistentMap)
			allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;

		vmaCreateBuffer(
			Renderer::GetPrimaryGPU()->vmaAllocator, 
			&createInfo, &allocationCreateInfo, 
			&impl.vkBuffer, 
			&impl.vmaAllocation, &impl.vmaAllocationInfo
		);

		if (data)
		{
			if (mConfig.type == BufferType::StagingBuffer || mConfig.type == BufferType::UniformBuffer)
			{
				if (!mConfig.persistentMap)
					Map();

				memcpy(GetData(), data, mConfig.size);

				if (!mConfig.persistentMap)
					UnMap();

				if (mConfig.type == BufferType::UniformBuffer)
					Flush();
			}
			else
			{
				SetDataUsingStagingBuffer(this, data);
			}
		}
	}

	Buffer::~Buffer()
	{
		vmaDestroyBuffer(Renderer::GetPrimaryGPU()->vmaAllocator, mConfig.impl->vkBuffer, mConfig.impl->vmaAllocation);
	}

	void Buffer::SetDataUsingStagingBuffer(Buffer* to, const void* data, uint32_t size, uint32_t offset, bool async)
	{
		BufferConfiguration config;
		config.size = size;
		config.cpuVisible = true;
		config.persistentMap = true;
		config.type = BufferType::StagingBuffer;

		Buffer stagingBuffer(config, data);

		Copy(to, &stagingBuffer, size, 0, offset, async);
	}

	void Buffer::Copy(Buffer* to, Buffer* from, uint32_t size, uint32_t srcOffset, uint32_t dstOffset, bool async)
	{
		size = glm::min(size, glm::min(to->mConfig.size, from->mConfig.size));

		VkBufferCopy region{};
		region.size = size;
		region.dstOffset = dstOffset;
		region.srcOffset = srcOffset;

		if (async)
		{
			CommandBuffers cb;
			cb.Init();
			cb.Begin();

			vkCmdCopyBuffer(cb.Get(), from->mConfig.impl->vkBuffer, to->mConfig.impl->vkBuffer, 1, &region);

			cb.End();
			cb.Submit();
		}
		else
		{
			vkCmdCopyBuffer(
				renderWindow->GetConfig().renderInfo->commandBuffer.GetCommandBuffer(), 
				from->mConfig.impl->vkBuffer, 
				to->mConfig.impl->vkBuffer, 
				1, &region
			);
		}

	}

	void* Buffer::GetData() const
	{
		return mConfig.impl->vmaAllocationInfo.pMappedData;
	}

	void Buffer::Map()
	{
		vmaMapMemory(Renderer::GetPrimaryGPU()->vmaAllocator, mConfig.impl->vmaAllocation, &mConfig.impl->vmaAllocationInfo.pMappedData);
	}

	void Buffer::UnMap()
	{
		vmaUnmapMemory(Renderer::GetPrimaryGPU()->vmaAllocator, mConfig.impl->vmaAllocation);
	}
	void Buffer::Flush()
	{
		vmaFlushAllocation(Renderer::GetPrimaryGPU()->vmaAllocator, mConfig.impl->vmaAllocation, 0, VK_WHOLE_SIZE);
	}

	Texture::Texture(const TextureConfiguration& config)
		: mConfig(config)
	{
		Init();
	}

	Texture::Texture(const char* file)
	{
		LoadFromFile(file);
	}

	Texture::Texture(const char* file, const TextureConfiguration& config)
		: mConfig(config)
	{
		LoadFromFile(file);
	}

	void Texture::LoadFromFile(const char* file)
	{
		int x, y, c;
		void* data = stbi_load(file, &x, &y, &c, STBI_rgb_alpha);
		mConfig.format.channels = 4;
		mConfig.size = { x, y };

		Init();

		SetData(data);
		stbi_image_free(data);
	}

	void Texture::Init()
	{
		mConfig.impl = new TextureImplementationInformation();
		auto& impl = *mConfig.impl;

		VkImageCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		createInfo.arrayLayers = 1;
		createInfo.extent = { mConfig.size.x, mConfig.size.y, 1 };
		createInfo.mipLevels = 1;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		createInfo.imageType = VK_IMAGE_TYPE_2D;

		// TODO
		createInfo.tiling = VK_IMAGE_TILING_LINEAR; 
		createInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		if (mConfig.type.Get<TextureType::copySrc>())
			createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		if (mConfig.type.Get<TextureType::copyDst>())
			createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		if (mConfig.type.Get<TextureType::sampled>())
			createInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

		if (mConfig.type.Get<TextureType::shaderOutput>())
			if (mConfig.format.isDepth)
				createInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			else
				createInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		createInfo.format = utils::to_vk_format(mConfig.format);

		// TODO host memory images
		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

		vmaCreateImage(Renderer::GetPrimaryGPU()->vmaAllocator, &createInfo, &allocInfo, &impl.vkImage, &impl.vmaAllocation, nullptr);

		if (mConfig.type.Get<TextureType::sampled>())
		{
			VkImageViewCreateInfo viewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			viewCreateInfo.format = createInfo.format;
			viewCreateInfo.image = impl.vkImage;
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewCreateInfo.subresourceRange.aspectMask = mConfig.format.isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			viewCreateInfo.subresourceRange.baseArrayLayer = 0;
			viewCreateInfo.subresourceRange.baseMipLevel = 0;
			viewCreateInfo.subresourceRange.layerCount = 1;
			viewCreateInfo.subresourceRange.levelCount = 1;

			VkResult err = vkCreateImageView(Renderer::GetPrimaryGPU()->vkDevice, &viewCreateInfo, nullptr, &impl.vkImageView);
			RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan {}: Failed to create image view", err);
		}

		// create imgui sampler if it doesn't exist
		if (!impl.imguiSampler)
		{
			VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.anisotropyEnable = VK_FALSE;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.minLod = 0.0f;
			samplerInfo.maxLod = 0.0f;
			samplerInfo.mipLodBias = 0.0f;

			VkResult err = vkCreateSampler(primaryGPU->vkDevice, &samplerInfo, nullptr, &impl.imguiSampler);
			RDR_ASSERT_MSG(err == VK_SUCCESS, "Vulkan {}: Failed to create imgui sampler", err);
		}
	}

	Texture::~Texture()
	{
		if (mConfig.impl->vkImageView)
			vkDestroyImageView(Renderer::GetPrimaryGPU()->vkDevice, mConfig.impl->vkImageView, nullptr);

		vmaDestroyImage(Renderer::GetPrimaryGPU()->vmaAllocator, mConfig.impl->vkImage, mConfig.impl->vmaAllocation);
	}

	size_t Texture::GetByteSize() const
	{
		size_t size = mConfig.size.x * mConfig.size.y;

		if (mConfig.format.isDepth)
			size *= 4;
		else
			size *= mConfig.format.channels;

		return size;
	}

	void Texture::SetData(const void* data, bool async)
	{
		BufferConfiguration bufferConfig;
		bufferConfig.size = (uint32_t)GetByteSize();
		bufferConfig.type = BufferType::StagingBuffer;

		Buffer stagingBuffer(bufferConfig, data);

		SetData(&stagingBuffer, async);
	}

	void Texture::SetData(const Buffer* buffer, bool async)
	{
		VkBufferImageCopy range{};
		range.imageExtent = { mConfig.size.x, mConfig.size.y, 1 };
		range.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.imageSubresource.baseArrayLayer = 0;
		range.imageSubresource.layerCount = 1;
		range.imageSubresource.mipLevel = 0;
			
		VkImageLayout finalLayout = mConfig.impl->vkImageLayout = mConfig.type.Get<TextureType::sampled>() ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		if (async)
		{
			CommandBuffers cb;
			cb.Init();
			cb.Begin();

			utils::transition_vk_image_layout(cb.Get(), mConfig.impl->vkImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			vkCmdCopyBufferToImage(cb.Get(), buffer->GetConfig().impl->vkBuffer, mConfig.impl->vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &range);

			utils::transition_vk_image_layout(cb.Get(), mConfig.impl->vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, finalLayout);

			cb.End();
			cb.Submit();
		}
		else
		{
			VkCommandBuffer cb = renderWindow->GetConfig().renderInfo->commandBuffer.GetCommandBuffer();
			utils::transition_vk_image_layout(cb, mConfig.impl->vkImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			vkCmdCopyBufferToImage(cb, buffer->GetConfig().impl->vkBuffer, mConfig.impl->vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &range);

			utils::transition_vk_image_layout(cb, mConfig.impl->vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, finalLayout);
		}
	}

	void Texture::LoadDataToBuffer(Buffer* buffer, bool async)
	{
		VkBufferImageCopy range{};
		range.imageExtent = { mConfig.size.x, mConfig.size.y, 1 };
		range.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.imageSubresource.baseArrayLayer = 0;
		range.imageSubresource.layerCount = 1;
		range.imageSubresource.mipLevel = 0;

		if (async)
		{
			CommandBuffers cb;
			cb.Init();
			cb.Begin();

			if (mConfig.impl->vkImageLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			{
				utils::transition_vk_image_layout(cb.Get(), mConfig.impl->vkImage, mConfig.impl->vkImageLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
				mConfig.impl->vkImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			}

			vkCmdCopyImageToBuffer(cb.Get(), mConfig.impl->vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer->GetConfig().impl->vkBuffer, 1, &range);

			if (mConfig.type.Get<TextureType::sampled>())
			{
				utils::transition_vk_image_layout(cb.Get(), mConfig.impl->vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				mConfig.impl->vkImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}

			cb.End();
			cb.Submit();
		}
		else
		{
			VkCommandBuffer cb = renderWindow->GetConfig().renderInfo->commandBuffer.GetCommandBuffer();
			if (mConfig.impl->vkImageLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			{
				utils::transition_vk_image_layout(cb, mConfig.impl->vkImage, mConfig.impl->vkImageLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
				mConfig.impl->vkImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			}

			vkCmdCopyImageToBuffer(cb, mConfig.impl->vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer->GetConfig().impl->vkBuffer, 1, &range);

			if (mConfig.type.Get<TextureType::sampled>())
			{
				utils::transition_vk_image_layout(cb, mConfig.impl->vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				mConfig.impl->vkImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
		}
	}

	void* Texture::GetImGuiID() const
	{
		auto& id = mConfig.impl->imguiDescriptorSet;
		return id ? id : (id = ImGui_ImplVulkan_AddTexture(TextureImplementationInformation::imguiSampler, mConfig.impl->vkImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
	}
}