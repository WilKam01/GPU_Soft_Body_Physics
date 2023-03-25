#pragma once

#include "Device.h"
#include "Buffer.h"

class CommandPool
{
private:
	VkCommandPool m_commandPool;
	Device* p_device;
public:
	void init(Device& device);
	void cleanup();

	VkCommandBuffer beginSingleTimeCommand();
	void endSingleTimeCommand(VkCommandBuffer buffer);
	void copyBuffer(Buffer& src, Buffer& dst, VkDeviceSize size);

	inline VkCommandPool get() { return m_commandPool; }
};

