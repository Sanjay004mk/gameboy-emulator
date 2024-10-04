#pragma once
#include "renderer/rdrapi.h"

#include "renderer/event.h

struct GLFWwindow;

namespace rdr
{
	enum class CursorState 
	{
		Normal        =  0x00034001,
		Hidden        =  0x00034002,
		Disabled      =  0x00034003,
	};

	struct WindowConfiguration
	{
		glm::u32vec2 size = glm::u32vec2(1280, 720);
		glm::u32vec2 position = glm::u32vec2(100, 100);

		std::string title;

		uint32_t refreshRate = 60;

        bool fullscreen = false;
        bool resizable = true;

        bool vsync = true;

		CursorState cursor = CursorState::Normal;

        bool minimized = false;
        bool maximized = false;

        bool decorated = true;
	};

	class RDRAPI Window
	{
	public:
		using EventCallbackFunction = std::function<void(Event& e)>;

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
		void SetVsync(bool isVsyncOn);
		void SetCursor(CursorState cursor);
		void SetMinimized(bool isMinimized);
		void SetMaximized(bool isMaximized);
		void SetDecorated(bool isDecorated);

		void SetupCallback(const EventCallbackFunction& callback);

	private:
		void Init();

		GLFWwindow* mGlfwWindow;
		WindowConfiguration mConfig;
		EventCallbackFunction mEventCallback;
	};
}