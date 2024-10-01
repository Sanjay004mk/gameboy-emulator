#include <iostream>

#include <renderer/renderer.h>

#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>


void fn()
{
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(400, 400, "Title", nullptr, nullptr);

    while (!glfwWindowShouldClose(window)) 
    {
        glfwWaitEvents();
    }
}