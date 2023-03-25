#pragma once

#include <vulkan/vulkan.h>

class Window;

class Instance
{
public:
#ifdef DEBUG
	const bool c_enableValidationLayers = true;
#else
	const bool c_enableValidationLayers = false;
#endif

	const std::vector<const char*> c_validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
private:
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	VkResult CreateDebugUtilsMessengerEXT(
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);

	bool checkValidationLayerSupport();
public:
	void init(Window& window);
	void cleanup();
	inline VkInstance get() { return m_instance; }
};

