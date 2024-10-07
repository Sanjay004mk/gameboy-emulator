#include <renderer/rdrpch.h>

#include "renderer/rdr.h"

#include "renderengine.h"

#include <glfw/glfw3.h>

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

	const GPUDeviceHandle rdr::Renderer::GetPrimaryGPU()
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
					mPrimaryDevice = new GPUDevice(device);
					break;
				}
			}

			// TODO better device selection
			if (!mPrimaryDevice) // default to first device
				mPrimaryDevice = new GPUDevice(devices[0]);
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

	GPUDevice::GPUDevice(VkPhysicalDevice device)
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
		}

		RDR_LOG_INFO("Vulkan: Created logical device");
	}

	GPUDevice::~GPUDevice()
	{
		vkDestroyDevice(mDevice, nullptr);
	}
}