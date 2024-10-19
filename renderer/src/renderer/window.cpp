#include "renderer/rdrpch.h"

#include "renderer/renderer.h"
#include "renderer/window.h"

#include <GLFW/glfw3.h>

namespace utils
{
	static void register_window_close_event(GLFWwindow* window)
	{
		glfwSetWindowCloseCallback(window, [](GLFWwindow* glfwWindow)
			{
				rdr::WindowConfiguration& config = *(rdr::WindowConfiguration*)glfwGetWindowUserPointer(glfwWindow);

				rdr::WindowCloseEvent event;
				config.eventCallbacks[rdr::EventType::WindowClose](event);
			});
	}

	static void register_framebuffer_resize_event(GLFWwindow* window)
	{
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow* glfwWindow, int32_t width, int32_t height)
			{
				rdr::WindowConfiguration& config = *(rdr::WindowConfiguration*)glfwGetWindowUserPointer(glfwWindow);
				config.size.x = (uint32_t)width;
				config.size.y = (uint32_t)height;

				config.windowResizeFn();

				rdr::WindowResizeEvent event(config.size.x, config.size.y);
				config.eventCallbacks[rdr::EventType::WindowResize](event);
			});
	}

	static void register_key_event(GLFWwindow* window)
	{
		glfwSetKeyCallback(window, [](GLFWwindow* glfwWindow, int32_t key, int32_t scancode, int32_t action, int32_t mods)
			{
				rdr::WindowConfiguration& config = *(rdr::WindowConfiguration*)glfwGetWindowUserPointer(glfwWindow);

				switch (action)
				{
				case GLFW_PRESS:
				{
					rdr::KeyPressedEvent event(key);
					config.eventCallbacks[rdr::EventType::KeyPressed](event);
					break;
				}
				case GLFW_RELEASE:
				{
					rdr::KeyReleasedEvent event(key);
					config.eventCallbacks[rdr::EventType::KeyReleased](event);
					break;
				}
				case GLFW_REPEAT:
				{
					rdr::KeyPressedEvent event(key, true);
					config.eventCallbacks[rdr::EventType::KeyPressed](event);
					break;
				}
				}

			});
	}

	static void register_key_typed_event(GLFWwindow* window)
	{
		glfwSetCharCallback(window, [](GLFWwindow* glfwWindow, uint32_t keycode)
			{
				rdr::WindowConfiguration& config = *(rdr::WindowConfiguration*)glfwGetWindowUserPointer(glfwWindow);
				
				rdr::KeyTypedEvent event(keycode);
				config.eventCallbacks[rdr::EventType::KeyTyped](event);
			});
	}

	static void register_mouse_button_event(GLFWwindow* window)
	{
		glfwSetMouseButtonCallback(window, [](GLFWwindow* glfwWindow, int32_t button, int32_t action, int32_t mods)
			{
				rdr::WindowConfiguration& config = *(rdr::WindowConfiguration*)glfwGetWindowUserPointer(glfwWindow);

				switch (action)
				{
				case GLFW_PRESS:
				{
					rdr::MouseButtonPressedEvent event(button);
					config.eventCallbacks[rdr::EventType::MouseButtonPressed](event);
					break;
				}
				case GLFW_RELEASE:
				{
					rdr::MouseButtonReleasedEvent event(button);
					config.eventCallbacks[rdr::EventType::MouseButtonReleased](event);
					break;
				}
				}
			});
	}

	static void register_mouse_scroll_event(GLFWwindow* window)
	{
		glfwSetScrollCallback(window, [](GLFWwindow* glfwWindow, double xOffs, double yOffs)
			{
				rdr::WindowConfiguration& config = *(rdr::WindowConfiguration*)glfwGetWindowUserPointer(glfwWindow);
				
				rdr::MouseScrolledEvent event((float)xOffs, (float)yOffs);
				config.eventCallbacks[rdr::EventType::MouseScrolled](event);
			});
	}

	static void register_mouse_move_event(GLFWwindow* window)
	{
		glfwSetCursorPosCallback(window, [](GLFWwindow* glfwWindow, double xPos, double yPos)
			{
				rdr::WindowConfiguration& config = *(rdr::WindowConfiguration*)glfwGetWindowUserPointer(glfwWindow);
				
				rdr::MouseMovedEvent event((float)xPos, (float)yPos);
				config.eventCallbacks[rdr::EventType::MouseMoved](event);
			});
	}

	using register_fn = void(*)(GLFWwindow*);
	static register_fn register_fns[] = {
		register_window_close_event,
		register_framebuffer_resize_event,
		register_key_event,
		register_key_typed_event,
		register_mouse_button_event,
		register_mouse_scroll_event,
		register_mouse_move_event,
	};

	static void register_all_events(GLFWwindow* window)
	{
		for (size_t i = 0; i < 7; i++)
			register_fns[i](window);
	}
}

namespace rdr
{
	Window::Window(const WindowConfiguration& conf)
		: mConfig(conf)
	{
		Init();
	}

	Window::~Window()
	{
		RDR_LOG_TRACE("Destroying Window: [{}]", mConfig.title);
		CleanupCommandUnit();
		CleanupSwapchain();
		CleanupSurface();
		glfwDestroyWindow(mGlfwWindow);
	}

	void Window::Init()
	{
		glfwWindowHint(GLFW_MAXIMIZED, mConfig.maximized);
		glfwWindowHint(GLFW_DECORATED, mConfig.decorated);
		glfwWindowHint(GLFW_REFRESH_RATE, mConfig.refreshRate);
		glfwWindowHint(GLFW_RESIZABLE, mConfig.resizable);

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		GLFWmonitor* monitor = mConfig.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
		mGlfwWindow = glfwCreateWindow(mConfig.size.x, mConfig.size.y, mConfig.title.c_str(), monitor, nullptr);

		if (mConfig.maximized) // update width and height as window is maximized
		{
			glm::ivec2 newSize{};
			glfwGetFramebufferSize(mGlfwWindow, &newSize.x, &newSize.y);
			mConfig.size = newSize;
		}

		if (!mConfig.maximized)
			glfwSetWindowPos(mGlfwWindow, mConfig.position.x, mConfig.position.y);

		glfwSetInputMode(mGlfwWindow, GLFW_CURSOR, (int)mConfig.cursor);

		if (mConfig.minimized)
			glfwIconifyWindow(mGlfwWindow);

		glfwSetWindowUserPointer(mGlfwWindow, &mConfig);

		glfwSetWindowMaximizeCallback(mGlfwWindow, [](GLFWwindow* glfwWindow, int32_t maximized)
			{
				WindowConfiguration& config = *(WindowConfiguration*)glfwGetWindowUserPointer(glfwWindow);

				config.maximized = maximized;
			});

		glfwSetWindowIconifyCallback(mGlfwWindow, [](GLFWwindow* glfwWindow, int32_t iconified)
			{
				WindowConfiguration& config = *(WindowConfiguration*)glfwGetWindowUserPointer(glfwWindow);

				config.minimized = iconified;
			});

		glfwSetWindowPosCallback(mGlfwWindow, [](GLFWwindow* glfwWindow, int32_t xpos, int32_t ypos)
			{
				WindowConfiguration& config = *(WindowConfiguration*)glfwGetWindowUserPointer(glfwWindow);

				config.position.x = (uint32_t)xpos;
				config.position.y = (uint32_t)ypos;
			});

		for (size_t i = 0; i < sizeof(mConfig.eventCallbacks) / sizeof(mConfig.eventCallbacks[0]); i++)
			if (!mConfig.eventCallbacks[i])
				mConfig.eventCallbacks[i] = [](Event& e) {};

		utils::register_all_events(mGlfwWindow);

		RDR_LOG_TRACE("Created Window: [{}, ({}, {})]", mConfig.title, mConfig.size.x, mConfig.size.y);

		if (!mConfig.gpuDevice)
			mConfig.gpuDevice = Renderer::GetPrimaryGPU();

		mConfig.windowResizeFn = [this]() { this->ResetSwapchain(); };

		SetupSurface();
		SetupSwapchain();
		SetupCommandUnit();
	}

	void Window::RegisterCallback(EventID eventType, EventCallbackFunction callback)
	{
		mConfig.eventCallbacks[eventType] = callback;
	}

	void Window::SetEventCallback(const EventCallbackFunction& callback)
	{
		for (size_t i = 0; i < sizeof(mConfig.eventCallbacks) / sizeof(mConfig.eventCallbacks[0]); i++)
			mConfig.eventCallbacks[i] = callback;
	}

	bool Window::ShouldClose() const
	{
		return glfwWindowShouldClose(mGlfwWindow);
	}

	void Window::SetSize(const glm::u32vec2& size)
	{
		mConfig.size = size;
		glfwSetWindowSize(mGlfwWindow, size.x, size.y);
	}

	void Window::SetPosition(const glm::u32vec2& position)
	{
		mConfig.position = position;
		glfwSetWindowPos(mGlfwWindow, position.x, position.y);
	}

	void Window::SetTitle(std::string_view title)
	{
		mConfig.title = title;
		glfwSetWindowTitle(mGlfwWindow, mConfig.title.data());
	}

	void Window::SetFullscreen(bool isFullscreen)
	{
		if (isFullscreen ^ mConfig.fullscreen) // there is a change in state
		{
			GLFWmonitor* monitor = isFullscreen ? glfwGetPrimaryMonitor() : nullptr;

			glfwSetWindowMonitor(mGlfwWindow, monitor, mConfig.position.x, mConfig.position.y, mConfig.size.x, mConfig.size.y, mConfig.refreshRate);
			
			mConfig.fullscreen = isFullscreen;
		}
	}

	void Window::SetResizable(bool isResizable)
	{
		mConfig.resizable = isResizable;
		glfwSetWindowAttrib(mGlfwWindow, GLFW_RESIZABLE, isResizable);
	}

	void Window::SetPresentMode(PresentMode mode)
	{
		mConfig.presentMode = mode;

		ResetSwapchain();
	}

	void Window::SetCursor(CursorState cursor)
	{
		mConfig.cursor = cursor;
		glfwSetInputMode(mGlfwWindow, GLFW_CURSOR, (int)cursor);
	}

	void Window::SetMinimized(bool isMinimized)
	{
		if (isMinimized ^ mConfig.minimized)
		{
			if (isMinimized)
				glfwIconifyWindow(mGlfwWindow);
			else
				glfwRestoreWindow(mGlfwWindow);
		
			mConfig.minimized = isMinimized;
		}
	}

	void Window::SetMaximized(bool isMaximized)
	{
		if (isMaximized ^ mConfig.maximized)
		{
			if (isMaximized)
				glfwMaximizeWindow(mGlfwWindow);
			else
				glfwRestoreWindow(mGlfwWindow);

			glm::ivec2 newSize{};
			glfwGetWindowSize(mGlfwWindow, &newSize.x, &newSize.y);
			mConfig.size = newSize;

			mConfig.maximized = isMaximized;
		}
	}

	void Window::SetDecorated(bool isDecorated)
	{
		if (isDecorated ^ mConfig.decorated)
		{
			glfwSetWindowAttrib(mGlfwWindow, GLFW_DECORATED, isDecorated);

			mConfig.decorated = isDecorated;
		}
	}

	bool Window::IsKeyDown(KeyCode key) const
	{
		return glfwGetKey(mGlfwWindow, key);
	}

	bool Window::IsMouseButtonDown(MouseCode button) const
	{
		return glfwGetMouseButton(mGlfwWindow, button);
	}

	glm::vec2 Window::GetMousePosition() const
	{
		double xpos, ypos;
		glfwGetCursorPos(mGlfwWindow, &xpos, &ypos);
		return glm::vec2((float)xpos, (float)ypos);
	}

	float Window::GetMouseX() const
	{
		return GetMousePosition().x;
	}

	float Window::GetMouseY() const
	{
		return GetMousePosition().y;
	}
}
