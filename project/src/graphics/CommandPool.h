#pragma once

#include "Device.h"
#include "Buffer.h"

class CommandPool
{
private:
	Device* p_device;

	VkCommandPool m_commandPool;
	VkPipelineBindPoint m_bindPoint;
public:
	void init(Device& device, VkPipelineBindPoint bindPoint);
	void cleanup();

	VkCommandBuffer beginSingleTimeCommand();
	void endSingleTimeCommand(VkCommandBuffer buffer);
	void copyBuffer(Buffer& src, Buffer& dst, VkDeviceSize size);

	inline VkCommandPool get() { return m_commandPool; }
};

