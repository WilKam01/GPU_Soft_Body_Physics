#pragma once

#include "CommandPool.h"

class CommandBufferArray
{
private:
	Device* p_device;
	CommandPool* p_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;
public:
	void init(Device& device, CommandPool& commandPool, const uint32_t& size);
	void cleanup();

	void begin(size_t i);
	void end(size_t i);

	inline uint32_t size() { return (uint32_t)m_commandBuffers.size(); }
	inline VkCommandBuffer& operator[](const size_t i) { return m_commandBuffers[i]; }
};

