#pragma once
#include "renderer/rdrapi.h"

#include "renderer/event.h"

struct GLFWwindow;

namespace rdr
{
	enum class CursorState 
	{
		Normal        =  0x00034001,
		Hidden        =  0x00034002,
		Disabled      =  0x00034003,
	};

	enum class PresentMode
	{
		NoVSync = 0,
		TripleBuffer = 1,
		VSync = 2,
		VSyncRelaxed = 3,
	};

	struct GPU;
	using GPUHandle = GPU*;

	struct WindowSurfaceInformation;

	struct WindowConfiguration
	{
		glm::u32vec2 size = glm::u32vec2(1280, 720);
		glm::u32vec2 position = glm::u32vec2(100, 100);

		std::string title;

		uint32_t refreshRate = 60;

        bool fullscreen = false;
        bool resizable = true;

        PresentMode presentMode = PresentMode::VSync;

		CursorState cursor = CursorState::Normal;

        bool minimized = false;
        bool maximized = false;

        bool decorated = true;

		EventCallbackFunction eventCallbacks[EventType::EventCount];
		std::function<void(void)> windowResizeFn = nullptr;

		GPUHandle gpuDevice = nullptr;
		WindowSurfaceInformation* surfaceInfo = nullptr;
	};	

	class RDRAPI Window
	{
	public:

		Window(const WindowConfiguration& conf = {});
		~Window();

		void Update();
		bool ShouldClose() const;

		const auto& GetConfig() const { return mConfig; }

		void SetSize(const glm::u32vec2& size);
		void SetPosition(const glm::u32vec2& position);
		void SetTitle(std::string_view title);
		void SetFullscreen(bool isFullscreen);
		void SetResizable(bool isResizable);
		void SetPresentMode(PresentMode presentMode);
		void SetCursor(CursorState cursor);
		void SetMinimized(bool isMinimized);
		void SetMaximized(bool isMaximized);
		void SetDecorated(bool isDecorated);

		void SetEventCallback(const EventCallbackFunction& callback);

		template <typename T, typename F>
		void RegisterCallback(F func)
		{
			RegisterCallback(T::GetStaticType(), [func](Event& e) { func(static_cast<T&>(e)); });
		}

		bool IsKeyDown(KeyCode key) const;
		bool IsMouseButtonDown(MouseCode button) const;

		glm::vec2 GetMousePosition() const;
		float GetMouseX() const;
		float GetMouseY() const;

		void ResetSwapchain();

	private:
		void Init();
		void SetupSurface();
		void SetupSwapchain();
		void CleanupSurface();
		void CleanupSwapchain();

		void RegisterCallback(EventID eventType, EventCallbackFunction callback);

		GLFWwindow* mGlfwWindow;
		WindowConfiguration mConfig;
	};
}