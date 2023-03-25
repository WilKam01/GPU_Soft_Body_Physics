#pragma once

#include "graphics/Renderer.h"

struct GLFWwindow;

class Window
{
private:
	GLFWwindow* m_window;
	std::string m_name;
	uint32_t m_width;
	uint32_t m_height;

	static void windowResizeCallback(GLFWwindow* window, int width, int height);
public:
	void init(std::string name, uint32_t width, uint32_t height);
	void cleanup();
	void update();
	void waitEvents();
	const bool isMinimized();

	const bool shouldClose();
	const std::vector<const char*> getRequiredExtensions();
	void createSurface(VkInstance instance, VkSurfaceKHR& surface);

	inline const std::string& getName() const { return m_name; }
	inline const uint32_t getWidth() const { return m_width; }
	inline const uint32_t getHeight() const { return m_height; }
};

