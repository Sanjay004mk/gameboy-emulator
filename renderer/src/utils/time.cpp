#include "renderer/rdrpch.h"

#include <glfw/glfw3.h>

#include "renderer/time.h"

namespace rdr
{
	float Time::GetTime()
	{
		return (float)glfwGetTime();
	}

	void Time::SetTime(float time)
	{
		glfwSetTime(time);
	}
}