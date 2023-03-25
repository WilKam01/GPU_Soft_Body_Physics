#include "pch.h"
#include "Device.h"

void Device::pickPhysicalDevice(Instance& instance)
{
	m_physicalDevice = VK_NULL_HANDLE;

	uint32_t count = 0;
	vkEnumeratePhysicalDevices(instance.get(), &count, nullptr);
	std::vector<VkPhysicalDevice> devices(count);
	vkEnumeratePhysicalDevices(instance.get(), &count, devices.data());

	for (const auto& device : devices)
	{
		if (isDeviceSuitable(device))
		{
			m_physicalDevice = device;
			m_msaaSamples = getMaxUsableSampleCount();
			break;
		}
	}

	if (m_physicalDevice == VK_NULL_HANDLE)
		LOG_ERROR("Failed to find suitable GPU!");
}

bool Device::isDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = findQueueFamilies(device);

	bool extensionsSupported = checkDeviceExtensionSupport(device);
	bool swapChainAdequate = true;
	if (extensionsSupported)
	{
		//TODO: Check swapchain support
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

QueueFamilyIndices Device::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &count, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = i;

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, *p_surface, &presentSupport);
		if (presentSupport)
			indices.presentFamily = i;

		if (indices.isComplete())
			break;
		i++;
	}

	return indices;
}

bool Device::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t count = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &count, availableExtensions.data());

	std::set<std::string> requiredExtensions(c_deviceExtensions.begin(), c_deviceExtensions.end());
	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

void Device::createLogicalDevice(Instance& instance)
{
	m_indices = findQueueFamilies(m_physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { m_indices.graphicsFamily.value(), m_indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = (uint32_t)(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = (uint32_t)(c_deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = c_deviceExtensions.data();

	createInfo.enabledLayerCount = 0;
	if (instance.c_enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(instance.c_validationLayers.size());
		createInfo.ppEnabledLayerNames = instance.c_validationLayers.data();
	}

	if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
		LOG_ERROR("Failed to create logical device!");

	vkGetDeviceQueue(m_device, m_indices.graphicsFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, m_indices.presentFamily.value(), 0, &m_presentQueue);
}

VkSampleCountFlagBits Device::getMaxUsableSampleCount()
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);

	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

void Device::init(Instance& instance, VkSurfaceKHR surface)
{
	p_surface = &surface;
	pickPhysicalDevice(instance);
	createLogicalDevice(instance);
}

void Device::cleanup()
{
	vkDestroyDevice(m_device, nullptr);
}

void Device::waitIdle()
{
	vkDeviceWaitIdle(m_device);
}
