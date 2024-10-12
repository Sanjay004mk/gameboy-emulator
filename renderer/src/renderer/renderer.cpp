#include <renderer/rdrpch.h>

#include "renderer/rdr.h"
#include "renderer/time.h"

#include "renderengine.h"

#include <glfw/glfw3.h>

#include <glm/glm.hpp>


namespace rdr
{

	Renderer::Renderer(const RendererConfiguration& config)
		: mConfig(config)
	{
		// GLFW initialization
		{
			RDR_LOG_INFO("Initializing GLFW");
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
		RDR_LOG_INFO("Shutting down Renderer");

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

	const GPUHandle rdr::Renderer::GetPrimaryGPU()
	{
		return mRenderer->mRenderEngine->primaryDevice;
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

			RDR_LOG_INFO("Created Vulkan Instance");
		}

		// Choosing Primary GPU
		{
			uint32_t count = 0;
			vkEnumeratePhysicalDevices(vkInstance, &count, nullptr);
			std::vector<VkPhysicalDevice> devices(count);
			vkEnumeratePhysicalDevices(vkInstance, &count, devices.data());

			RDR_ASSERT_NO_MSG_BREAK(count != 0);

			VkPhysicalDeviceProperties properties{};
			for (auto& device : devices)
			{
				vkGetPhysicalDeviceProperties(device, &properties);
				if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					RDR_LOG_INFO("Vulkan: Using {} as primary GPU", properties.deviceName);
					primaryDevice = new GPU(vkInstance, device);
					break;
				}
			}

			// TODO better device selection
			if (!primaryDevice) // default to first device
				primaryDevice = new GPU(vkInstance, devices[0]);
		}
	}

	RenderEngine::~RenderEngine()
	{
		delete primaryDevice;

#if defined(RDR_DEBUG)
		auto FNvkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugUtilsMessengerEXT");
		RDR_ASSERT_NO_MSG_BREAK(FNvkDestroyDebugUtilsMessengerEXT);

		FNvkDestroyDebugUtilsMessengerEXT(vkInstance, vkDebugUtilsMessenger, nullptr);
#endif

		vkDestroyInstance(vkInstance, nullptr);
		RDR_LOG_INFO("Destroyed Vulkan instance");
	}

	GPU::GPU(VkInstance vkInstance, VkPhysicalDevice device)
		: vkPhysicalDevice(device)
	{
		VkResult err{};
		// Graphics queue
		{
			uint32_t count = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
			std::vector<VkQueueFamilyProperties> properties(count);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());

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

		RDR_LOG_INFO("Vulkan: Created logical device");

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
	}

	GPU::~GPU()
	{
		vmaDestroyAllocator(vmaAllocator);
		vkDestroyDevice(vkDevice, nullptr);
	}

	void Window::SetupSurface()
	{
		mConfig.surfaceInfo = new WindowRenderInformation();
		VkSurfaceKHR& surface = mConfig.surfaceInfo->vkSurface;
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
		VkSurfaceKHR& surface = mConfig.surfaceInfo->vkSurface;
		VkSwapchainKHR& swapchain = mConfig.surfaceInfo->vkSwapchain;
		VkResult err{};

		VkSurfaceFormatKHR useFormat{};
		{
			bool surfaceFormatSet = false;

			std::array<VkFormat, 4> formats = { 
				VK_FORMAT_B8G8R8A8_UNORM, 
				VK_FORMAT_R8G8B8A8_SNORM, 
				VK_FORMAT_B8G8R8_UNORM, 
				VK_FORMAT_R8G8B8_SNORM 
			};
			VkColorSpaceKHR requiredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

			uint32_t count = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, nullptr);
			std::vector<VkSurfaceFormatKHR> surfaceFormats(count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, surfaceFormats.data());
			RDR_ASSERT_NO_MSG_BREAK(count != 0);

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
			std::vector<VkPresentModeKHR> presentModesAvailable(count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, presentModesAvailable.data());
			RDR_ASSERT_NO_MSG_BREAK(count != 0);

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
		if (extent.width == UINT32_MAX)
		{
			extent = {
				glm::clamp(mConfig.size.x, surfaceCapabilities.maxImageExtent.width, surfaceCapabilities.maxImageExtent.width),
				glm::clamp(mConfig.size.y, surfaceCapabilities.maxImageExtent.height, surfaceCapabilities.maxImageExtent.height)
			};
		}

		VkSwapchainCreateInfoKHR swapchainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		swapchainCreateInfo.imageFormat = useFormat.format;
		swapchainCreateInfo.imageColorSpace = useFormat.colorSpace;
		swapchainCreateInfo.presentMode = usePresentMode;
		swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount;
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

		mConfig.surfaceInfo->swapchainImages.resize(imageCount);
		uint32_t index = 0;
		for (auto& [image, view] : mConfig.surfaceInfo->swapchainImages)
		{
			image = imageViewCreateInfo.image = images[index];

			err = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &view);
			RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan {}: Failed to create image view", err);
		}		
	}

	void Window::SetupCommandUnit()
	{
		size_t imageCount = mConfig.surfaceInfo->swapchainImages.size();
		RDR_ASSERT_MSG_BREAK(imageCount != 0, "Vulkan: Creating command buffers before swapchain is created");

		VkResult err{};
		VkDevice vkDevice = mConfig.gpuDevice->vkDevice;

		WindowCommandUnit& cb = mConfig.surfaceInfo->commandBuffer;
		cb.vkCommandBuffers.resize(imageCount);
		cb.vkFences.resize(imageCount);
		cb.vkImageAcquiredSemaphores.resize(imageCount);
		cb.vkRenderFinishedSemaphores.resize(imageCount);

		VkCommandPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolCreateInfo.queueFamilyIndex = mConfig.gpuDevice->vkGraphicsQueueIndex;

		err = vkCreateCommandPool(vkDevice, &poolCreateInfo, nullptr, &cb.vkCommandPool);
		RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan {}: Failed to create command pool", err);

		VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocInfo.commandBufferCount = (uint32_t)imageCount;
		allocInfo.commandPool = cb.vkCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		err = vkAllocateCommandBuffers(vkDevice, &allocInfo, cb.vkCommandBuffers.data());
		RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan {}: Failed to create command buffer", err);

		VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

		for (size_t i = 0; i < imageCount; i++)
		{
			err = vkCreateFence(vkDevice, &fenceCreateInfo, nullptr, &cb.vkFences[i]);
			RDR_ASSERT_NO_MSG_BREAK(err == VK_SUCCESS);

			err = vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, nullptr, &cb.vkImageAcquiredSemaphores[i]);
			RDR_ASSERT_NO_MSG_BREAK(err == VK_SUCCESS);

			err = vkCreateSemaphore(vkDevice, &semaphoreCreateInfo, nullptr, &cb.vkRenderFinishedSemaphores[i]);
			RDR_ASSERT_NO_MSG_BREAK(err == VK_SUCCESS);
		}
	}

	void Window::CleanupSurface()
	{
		VkSurfaceKHR& surface = mConfig.surfaceInfo->vkSurface;
		vkDestroySurfaceKHR(Renderer::Get()->mRenderEngine->vkInstance, surface, nullptr);

		delete mConfig.surfaceInfo;
	}

	void Window::CleanupSwapchain()
	{
		for (auto& [image, view] : mConfig.surfaceInfo->swapchainImages)
			vkDestroyImageView(mConfig.gpuDevice->vkDevice, view, nullptr);
		
		vkDestroySwapchainKHR(mConfig.gpuDevice->vkDevice, mConfig.surfaceInfo->vkSwapchain, nullptr);

		mConfig.surfaceInfo->vkSwapchain = nullptr;
	}

	void Window::CleanupCommandUnit()
	{
		VkDevice vkDevice = mConfig.gpuDevice->vkDevice;

		vkDeviceWaitIdle(vkDevice);

		WindowCommandUnit& cb = mConfig.surfaceInfo->commandBuffer;

		vkFreeCommandBuffers(vkDevice, cb.vkCommandPool, (uint32_t)cb.vkCommandBuffers.size(), cb.vkCommandBuffers.data());
		cb.vkCommandBuffers.clear();

		vkDestroyCommandPool(vkDevice, cb.vkCommandPool, nullptr);

		for (auto& fence : cb.vkFences)
			vkDestroyFence(vkDevice, fence, nullptr);
		cb.vkFences.clear();

		for (auto& semaphore : cb.vkImageAcquiredSemaphores)
			vkDestroySemaphore(vkDevice, semaphore, nullptr);
		cb.vkImageAcquiredSemaphores.clear();

		for (auto& semaphore : cb.vkRenderFinishedSemaphores)
			vkDestroySemaphore(vkDevice, semaphore, nullptr);
		cb.vkRenderFinishedSemaphores.clear();
	}

	void Window::ResetSwapchain()
	{
		VkSwapchainKHR oldSwapchain = mConfig.surfaceInfo->vkSwapchain;
		size_t oldImageCount = mConfig.surfaceInfo->swapchainImages.size();

		for (auto& [image, view] : mConfig.surfaceInfo->swapchainImages)
			vkDestroyImageView(mConfig.gpuDevice->vkDevice, view, nullptr);

		SetupSwapchain();

		if (mConfig.surfaceInfo->swapchainImages.size() != oldImageCount)
		{
			CleanupCommandUnit();
			SetupCommandUnit();
		}

		vkDestroySwapchainKHR(mConfig.gpuDevice->vkDevice, oldSwapchain, nullptr);
	}

	void Window::Update()
	{
		// tmp
		WindowCommandUnit& cb = mConfig.surfaceInfo->commandBuffer;
		uint32_t& index = mConfig.surfaceInfo->imageIndex;

		vkWaitForFences(mConfig.gpuDevice->vkDevice, 1, &cb.GetFence(), VK_TRUE, UINT64_MAX);

		vkAcquireNextImageKHR(
			mConfig.gpuDevice->vkDevice, 
			mConfig.surfaceInfo->vkSwapchain, 
			UINT64_MAX, 
			cb.GetImageSemaphore(),
			VK_NULL_HANDLE, 
			&index
		);

		vkResetFences(mConfig.gpuDevice->vkDevice, 1, &cb.GetFence());

		VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkResetCommandBuffer(cb.GetCommandBuffer(), 0);

		VkImage& image = mConfig.surfaceInfo->swapchainImages[index].first;
		VkImageView& view = mConfig.surfaceInfo->swapchainImages[index].second;
		vkBeginCommandBuffer(cb.GetCommandBuffer(), &beginInfo);

		VkClearColorValue clearColor = {
			glm::abs(glm::sin(Time::GetTime())),
			glm::abs(glm::sin(Time::GetTime() + glm::radians(45.f))),
			glm::abs(glm::cos(Time::GetTime())),
			1.0f
		};

		VkImageSubresourceRange range{};
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseArrayLayer = 0;
		range.baseMipLevel = 0;
		range.layerCount = 1;
		range.levelCount = 1;

		VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_NONE;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.subresourceRange = range;
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
			&clearColor, 
			1, 
			&range);

		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		barrier.subresourceRange = range;
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

		vkEndCommandBuffer(cb.GetCommandBuffer());

		VkPipelineStageFlags dstStageMask[] = { VK_PIPELINE_STAGE_TRANSFER_BIT };

		VkSubmitInfo info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		info.commandBufferCount = 1;
		info.pCommandBuffers = &cb.GetCommandBuffer();
		info.signalSemaphoreCount = 1;
		info.pSignalSemaphores = &cb.GetRenderSemaphore();
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &cb.GetImageSemaphore();
		info.pWaitDstStageMask = dstStageMask;

		VkResult err = vkQueueSubmit(mConfig.gpuDevice->vkGraphicsQueue, 1, &info, cb.GetFence());

		vkQueueWaitIdle(mConfig.gpuDevice->vkGraphicsQueue);

		VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &mConfig.surfaceInfo->vkSwapchain;
		presentInfo.pImageIndices = &index;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &cb.GetRenderSemaphore();

		vkQueuePresentKHR(mConfig.gpuDevice->vkGraphicsQueue, &presentInfo);

		cb++;
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
		switch (config.type)
		{
		case BufferType::StagingBuffer:
			createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			break;

		case BufferType::GPUBuffer:
			createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			break;

		case BufferType::UniformBuffer:
			createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			break;

		case BufferType::VertexBuffer:
			createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			break;

		case BufferType::IndexBuffer:
			createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			break;
		}

		vmaCreateBuffer(
			Renderer::GetPrimaryGPU()->vmaAllocator, 
			&createInfo, &allocationCreateInfo, 
			&impl.vkBuffer, 
			&impl.vmaAllocation, &impl.vmaAllocationInfo
		);
	}

	Buffer::~Buffer()
	{

	}

	Texture::Texture(const TextureConfiguration& config)
		: mConfig(config)
	{

	}

	Texture::~Texture()
	{

	}
}