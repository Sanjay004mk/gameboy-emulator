#include "renderer/rdrpch.h"

#include "renderer/window.h"

#include <GLFW/glfw3.h>

namespace rdr
{
	Window::Window(const WindowConfiguration& conf)
		: mConfig(conf)
	{
		Init();
	}

	Window::~Window()
	{
		RDR_LOG_INFO("Destroying Window: [{}]", mConfig.title);
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

		glm::ivec2 newSize{};
		glfwGetWindowSize(mGlfwWindow, &newSize.x, &newSize.y);
		mConfig.size = newSize;

		if (!mConfig.maximized)
			glfwSetWindowPos(mGlfwWindow, mConfig.position.x, mConfig.position.y);

		glfwSetInputMode(mGlfwWindow, GLFW_CURSOR, (int)mConfig.cursor);

		if (mConfig.minimized)
			glfwIconifyWindow(mGlfwWindow);

		RDR_LOG_INFO("Created Window: [{}, ({}, {})]", mConfig.title, mConfig.size.x, mConfig.size.y);
	}

	void Window::SetupCallback(const EventCallbackFunction& callback)
	{
		mEventCallback = callback;

		glfwSetWindowUserPointer(mGlfwWindow, this);

		glfwSetFramebufferSizeCallback(mGlfwWindow, [](GLFWwindow* glfwWindow, int32_t width, int32_t height)
			{
				Window& window = *(Window*)glfwGetWindowUserPointer(glfwWindow);
			});
	}

	void Window::Update()
	{

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

	void Window::SetVsync(bool isVsyncOn)
	{
		// TODO
		mConfig.vsync = isVsyncOn;
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
}
