#pragma once

#include "Instance.h"

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

class Device
{
private:
	VkPhysicalDevice m_physicalDevice;
	VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	VkSurfaceKHR* p_surface;

	VkDevice m_device;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	QueueFamilyIndices m_indices;

	const std::vector<const char*> c_deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	void pickPhysicalDevice(Instance& instance);
	bool isDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	void createLogicalDevice(Instance& instance);
	VkSampleCountFlagBits getMaxUsableSampleCount();
public:
	void init(Instance& instance, VkSurfaceKHR surface);
	void cleanup();

	void waitIdle();
	inline VkDevice getLogical() { return m_device; }
	inline VkPhysicalDevice getPhysical() { return m_physicalDevice; }
	inline VkQueue getGraphicsQueue() { return m_graphicsQueue; }
	inline VkQueue getPresentQueue() { return m_presentQueue; }
	inline QueueFamilyIndices getQueueFamilyIndices() { return m_indices; }
	inline VkSampleCountFlagBits getMsaaSamples() { return m_msaaSamples; }
};

