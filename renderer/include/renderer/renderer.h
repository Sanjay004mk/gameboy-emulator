#pragma once

#include "renderer/rdrapi.h"
#include "renderer/window.h"
#include "renderer/texture.h"

#include <span>
#include <string_view>

namespace rdr
{
	struct CommandLineArgumentsList
	{
		CommandLineArgumentsList() = default;
		CommandLineArgumentsList(int32_t argc, const char** argv)
			: args(argv, argc)
		{ }

		constexpr auto begin() const { return args.begin(); }
		constexpr auto end() const { return args.end(); }
		template <typename T = size_t> const char* operator[](T index) const { return args[index]; }

		operator bool() const { return args.size() > 1; }
		
		std::span<const char*> args;
	};

	struct RenderEngine;
	struct GPU;
	using GPUHandle = GPU*;

	struct RendererConfiguration
	{
		CommandLineArgumentsList clArgs;
		std::string applicationName;
	};

	class Renderer
	{
	public:
		static RDRAPI void Init(int argc, const char** argv, std::string_view applicationName = "Application");
		static RDRAPI void Shutdown();

		static RDRAPI void PollEvents();
		static RDRAPI void WaitEvents();

		static RDRAPI Window* InstantiateWindow(const WindowConfiguration& windowConfig);
		static RDRAPI void FreeWindow(Window* window);

		static RDRAPI void BeginFrame(Window* window);
		static RDRAPI void EndFrame();
		static RDRAPI void Blit(Texture* srcTexture, Texture* dstTexture, const TextureBlitInformation& blitInfo);
		static RDRAPI void BlitToWindow(Texture* texture, const TextureBlitInformation& blitInfo);

		// TODO add ability to choose custom device and multiple devices
		static RDRAPI const GPUHandle GetPrimaryGPU(); //!! Set primary gpu for any new threads created before calling
		static RDRAPI void SetPrimaryGPU(GPUHandle gpu); // sets primary gpu for the calling thread only
		static RDRAPI const std::vector<GPUHandle>& GetAllGPUs();
		static RDRAPI void InitGPU(GPUHandle gpu);
		static RDRAPI bool IsActive(GPUHandle gpu);

		// TODO add CreateNewRenderThread fn that initializes thread_local variables

	private:
		Renderer(const RendererConfiguration& config);
		~Renderer();

		static Renderer* Get() { return mRenderer; }

		static inline Renderer* mRenderer = nullptr;
		RendererConfiguration mConfig;
		RenderEngine* mRenderEngine;
		std::vector<Window*> mWindows;

		friend class Window;
	};
}