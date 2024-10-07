#pragma once
#include "renderer/renderer.h"

#include <vulkan/vulkan.h>

namespace rdr
{
	class GPUDevice
	{
	public:
		GPUDevice(VkPhysicalDevice device);
		~GPUDevice();

	private:
		VkPhysicalDevice mPhysicalDevice = nullptr;
		VkDevice mDevice = nullptr;

		uint32_t mGraphicsQueueIndex = -1;
	};

	class RenderEngine
	{
	public:
		RenderEngine(const RendererConfiguration& config);
		~RenderEngine();

	private:
		VkInstance mVkInstance = nullptr;

#if defined(RDR_DEBUG)
		VkDebugUtilsMessengerEXT mDebugUtilsMessenger = nullptr;
#endif

		GPUDevice* mPrimaryDevice = nullptr;
		GPUDevice* mWorkerDevices = nullptr;

		friend class Renderer;
	};
}

template <typename ostream>
ostream& operator<<(ostream& stream, VkResult result)
{
	return stream << (int)result;
}