#pragma once

#include "Device.h"

class CommandPool
{
private:
	VkCommandPool m_commandPool;
	Device* p_device;
public:
	void init(Device& device);
	void cleanup();

	void beginSingleTimeCommand(VkCommandBuffer& buffer);
	void endSingleTimeCommand(VkCommandBuffer& buffer);

	inline VkCommandPool get() { return m_commandPool; }
};

