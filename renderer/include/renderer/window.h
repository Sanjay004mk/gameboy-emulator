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

		EventCallbackFunction eventCallbacks[EventType::EventCount];
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
		void SetVsync(bool isVsyncOn);
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

	private:
		void Init();
		void RegisterCallback(EventID eventType, EventCallbackFunction callback);

		GLFWwindow* mGlfwWindow;
		WindowConfiguration mConfig;
	};
}