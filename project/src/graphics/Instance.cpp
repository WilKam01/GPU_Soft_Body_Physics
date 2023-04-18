#include "pch.h"
#include "Instance.h"
#include "core/Window.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	LOG_WARNING(pCallbackData->pMessage);
	return VK_FALSE;
}

void Instance::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType =
		//VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

VkResult Instance::CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
		return func(m_instance, pCreateInfo, pAllocator, pDebugMessenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void Instance::DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
		func(m_instance, debugMessenger, pAllocator);
}

bool Instance::checkValidationLayerSupport()
{
	uint32_t count = 0;
	vkEnumerateInstanceLayerProperties(&count, nullptr);
	std::vector<VkLayerProperties> availableLayers(count);
	vkEnumerateInstanceLayerProperties(&count, availableLayers.data());

	for (const char* name : c_validationLayers)
	{
		bool found = false;

		for (const auto& props : availableLayers)
		{
			if (strcmp(name, props.layerName) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
			return false;
	}

	return true;
}

void Instance::init(Window& window)
{
	if (c_enableValidationLayers && !checkValidationLayerSupport())
		LOG_ERROR("Requested validation layers not available!");

	VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pEngineName = "Thesis";
	appInfo.pApplicationName = window.getName().c_str();
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appInfo;

	std::vector<const char*> reqExtensions = window.getRequiredExtensions();
	if (c_enableValidationLayers)
		reqExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	createInfo.enabledExtensionCount = (uint32_t)reqExtensions.size();
	createInfo.ppEnabledExtensionNames = reqExtensions.data();
	createInfo.enabledLayerCount = 0;
	createInfo.pNext = nullptr;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (c_enableValidationLayers)
	{
		createInfo.enabledLayerCount = (uint32_t)c_validationLayers.size();
		createInfo.ppEnabledLayerNames = c_validationLayers.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}

	if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
		LOG_ERROR("Failed to create instance!");

#ifdef DEBUG
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	LOG_WRITE("available extensions:");
	for (const auto& extension : extensions) {
		LOG_WRITE("\t" << extension.extensionName);
	}
#endif

	if (c_enableValidationLayers)
	{
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
		populateDebugMessengerCreateInfo(debugCreateInfo);

		if (CreateDebugUtilsMessengerEXT(&debugCreateInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
			LOG_ERROR("Failed to set up debug messenger!");
	}
}

void Instance::cleanup()
{
	if (c_enableValidationLayers)
		DestroyDebugUtilsMessengerEXT(m_debugMessenger, nullptr);

	vkDestroyInstance(m_instance, nullptr);
}
