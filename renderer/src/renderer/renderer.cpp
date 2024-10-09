#include <renderer/rdrpch.h>

#include "renderer/rdr.h"
#include "renderer/time.h"

#include "renderengine.h"

#include <glfw/glfw3.h>

#include <glm/glm.hpp>


namespace rdr
{
	// tmp
	static VkCommandPool pool;
	static std::vector<VkCommandBuffer> commandBuffers;
	static VkFence fence;

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
		return mRenderer->mRenderEngine->mPrimaryDevice;
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

			err = vkCreateInstance(&instanceCreateInfo, nullptr, &mVkInstance);
			RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan {}: Failed to create VkInstance", err);

#if defined(RDR_DEBUG)

			auto FNvkCreateDebugUtilsMessengerExt = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mVkInstance, "vkCreateDebugUtilsMessengerEXT");
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

			err = FNvkCreateDebugUtilsMessengerExt(mVkInstance, &debugCreateInfo, nullptr, &mDebugUtilsMessenger);
			RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan {}: Failed to create Debug messenger");
#endif

			RDR_LOG_INFO("Created Vulkan Instance");
		}

		// Choosing Primary GPU
		{
			uint32_t count = 0;
			vkEnumeratePhysicalDevices(mVkInstance, &count, nullptr);
			std::vector<VkPhysicalDevice> devices(count);
			vkEnumeratePhysicalDevices(mVkInstance, &count, devices.data());

			RDR_ASSERT_NO_MSG_BREAK(count != 0);

			VkPhysicalDeviceProperties properties{};
			for (auto& device : devices)
			{
				vkGetPhysicalDeviceProperties(device, &properties);
				if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					RDR_LOG_INFO("Vulkan: Using {} as primary GPU", properties.deviceName);
					mPrimaryDevice = new GPU(device);
					break;
				}
			}

			// TODO better device selection
			if (!mPrimaryDevice) // default to first device
				mPrimaryDevice = new GPU(devices[0]);
		}
	}

	RenderEngine::~RenderEngine()
	{
		delete mPrimaryDevice;

#if defined(RDR_DEBUG)
		auto FNvkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mVkInstance, "vkDestroyDebugUtilsMessengerEXT");
		RDR_ASSERT_NO_MSG_BREAK(FNvkDestroyDebugUtilsMessengerEXT);

		FNvkDestroyDebugUtilsMessengerEXT(mVkInstance, mDebugUtilsMessenger, nullptr);
#endif

		vkDestroyInstance(mVkInstance, nullptr);
		RDR_LOG_INFO("Destroyed Vulkan instance");
	}

	GPU::GPU(VkPhysicalDevice device)
		: mPhysicalDevice(device)
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
					mGraphicsQueueIndex = queueIndex;
					break;
				}
			}

			RDR_ASSERT_MSG_BREAK(mGraphicsQueueIndex != -1, "Vulkan: Failed to find suitable graphics queue");
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
			queueCreateInfo.queueFamilyIndex = mGraphicsQueueIndex;
			queueCreateInfo.pQueuePriorities = priorities;

			VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
			deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
			deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
			deviceCreateInfo.queueCreateInfoCount = 1;
			deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
			
			err = vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mDevice);
			RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan {}: Failed to create logical device", err);

			vkGetDeviceQueue(mDevice, mGraphicsQueueIndex, 0, &mGraphicsQueue);
		}

		RDR_LOG_INFO("Vulkan: Created logical device");

		// tmp
		{
			VkCommandPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
			poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			poolCreateInfo.queueFamilyIndex = mGraphicsQueueIndex;
			err = vkCreateCommandPool(mDevice, &poolCreateInfo, nullptr, &pool);
			RDR_ASSERT_NO_MSG_BREAK(err == VK_SUCCESS);

			VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
			
			vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &fence);
		}
	}

	GPU::~GPU()
	{
		// tmp
		{
			if (commandBuffers.size() > 0)
				vkFreeCommandBuffers(mDevice, pool, (uint32_t)commandBuffers.size(), commandBuffers.data());

			vkDestroyFence(mDevice, fence, nullptr);

			vkDestroyCommandPool(mDevice, pool, nullptr);
		}

		vkDestroyDevice(mDevice, nullptr);
	}

	void Window::SetupSurface()
	{
		mConfig.surfaceInfo = new WindowSurfaceInformation();
		VkSurfaceKHR& surface = mConfig.surfaceInfo->vkSurface;
		VkResult err = glfwCreateWindowSurface(Renderer::Get()->mRenderEngine->mVkInstance, mGlfwWindow, nullptr, &surface);
		RDR_ASSERT_MSG_BREAK(err == VK_SUCCESS, "Vulkan|GLFW {}: Failed to create window surface", err);

		VkBool32 supported = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(mConfig.gpuDevice->mPhysicalDevice, mConfig.gpuDevice->mGraphicsQueueIndex, surface, &supported);
		RDR_ASSERT_MSG_BREAK(supported == VK_TRUE, "Vulkan: Device doesn't support required window format");

		SetupSwapchain();
	}

	void Window::SetupSwapchain()
	{
		VkPhysicalDevice& physicalDevice = mConfig.gpuDevice->mPhysicalDevice;
		VkDevice& device = mConfig.gpuDevice->mDevice;
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

		// tmp
		{

			// tmp
			if (commandBuffers.size() > 0)
			{
				vkFreeCommandBuffers(mConfig.gpuDevice->mDevice, pool, (uint32_t)commandBuffers.size(), commandBuffers.data());
				commandBuffers.resize(0);
			}

			commandBuffers.resize(mConfig.surfaceInfo->swapchainImages.size());

			VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
			allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();
			allocInfo.commandPool = pool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			err = vkAllocateCommandBuffers(mConfig.gpuDevice->mDevice, &allocInfo, commandBuffers.data());

			
		}
	}

	void Window::CleanupSurface()
	{
		CleanupSwapchain();

		VkSurfaceKHR& surface = mConfig.surfaceInfo->vkSurface;
		vkDestroySurfaceKHR(Renderer::Get()->mRenderEngine->mVkInstance, surface, nullptr);

		delete mConfig.surfaceInfo;
	}

	void Window::CleanupSwapchain()
	{
		for (auto& [image, view] : mConfig.surfaceInfo->swapchainImages)
		{
			vkDestroyImageView(mConfig.gpuDevice->mDevice, view, nullptr);
		}

		vkDestroySwapchainKHR(mConfig.gpuDevice->mDevice, mConfig.surfaceInfo->vkSwapchain, nullptr);

		mConfig.surfaceInfo->vkSwapchain = nullptr;
	}

	void Window::ResetSwapchain()
	{
		VkSwapchainKHR oldSwapchain = mConfig.surfaceInfo->vkSwapchain;

		for (auto& [image, view] : mConfig.surfaceInfo->swapchainImages)
		{
			vkDestroyImageView(mConfig.gpuDevice->mDevice, view, nullptr);
		}

		SetupSwapchain();

		vkDestroySwapchainKHR(mConfig.gpuDevice->mDevice, oldSwapchain, nullptr);
	}

	void Window::Update()
	{
		// tmp

		vkResetFences(mConfig.gpuDevice->mDevice, 1, &fence);

		uint32_t index;
		vkAcquireNextImageKHR(
			mConfig.gpuDevice->mDevice, 
			mConfig.surfaceInfo->vkSwapchain, 
			UINT64_MAX, 
			VK_NULL_HANDLE, 
			fence, 
			&index
		);

		vkWaitForFences(mConfig.gpuDevice->mDevice, 1, &fence, VK_FALSE, UINT64_MAX);

		{
			uint32_t i = index;

			VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			vkResetCommandBuffer(commandBuffers[i], 0);

			VkImage& image = mConfig.surfaceInfo->swapchainImages[i].first;
			VkImageView& view = mConfig.surfaceInfo->swapchainImages[i].second;
			vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

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
				commandBuffers[i],
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			vkCmdClearColorImage(
				commandBuffers[i], 
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
				commandBuffers[i],
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			vkEndCommandBuffer(commandBuffers[i]);
		}

		VkSubmitInfo info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		info.commandBufferCount = 1;
		info.pCommandBuffers = &commandBuffers[index];

		VkResult err = vkQueueSubmit(mConfig.gpuDevice->mGraphicsQueue, 1, &info, VK_NULL_HANDLE);

		vkQueueWaitIdle(mConfig.gpuDevice->mGraphicsQueue);

		VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &mConfig.surfaceInfo->vkSwapchain;
		presentInfo.pImageIndices = &index;

		vkQueuePresentKHR(mConfig.gpuDevice->mGraphicsQueue, &presentInfo);
	}
}