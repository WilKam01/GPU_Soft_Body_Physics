#include "pch.h"
#include "Window.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

void Window::windowResizeCallback(GLFWwindow* window, int width, int height)
{
    Window* userWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    userWindow->m_width = width;
    userWindow->m_height = height;
}

void Window::init(std::string name, uint32_t width, uint32_t height)
{
    m_name = name;
    m_width = width;
    m_height = height;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(m_width, m_height, m_name.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, windowResizeCallback);
}

void Window::cleanup()
{
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Window::update()
{
    glfwPollEvents();
}

void Window::waitEvents()
{
    glfwWaitEvents();
}

const bool Window::isMinimized()
{
    return m_width == 0 || m_height == 0;
}

const bool Window::shouldClose()
{
    return glfwWindowShouldClose(m_window);
}

const std::vector<const char*> Window::getRequiredExtensions()
{
    uint32_t count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    return std::vector<const char*>(extensions, extensions + count);
}

void Window::createSurface(VkInstance instance, VkSurfaceKHR& surface)
{
    if (glfwCreateWindowSurface(instance, m_window, nullptr, &surface) != VK_SUCCESS)
        LOG_ERROR("Failed to create window surface!");
}
