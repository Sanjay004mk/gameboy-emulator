#pragma once
#include "renderer/renderer.h"

#include <vulkan/vulkan.h>

namespace rdr
{
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
	};
}

template <typename ostream>
ostream& operator<<(ostream& stream, VkResult result)
{
	return stream << (int)result;
}