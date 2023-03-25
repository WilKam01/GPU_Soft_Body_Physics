#pragma once

#include "Device.h"

class Buffer
{
private:
	VkBuffer m_buffer;
	VkDeviceMemory m_memory;
	VkDeviceSize m_alignment;
	VkDeviceSize m_size;
	VkBufferUsageFlags m_usageFlags;
	VkMemoryPropertyFlags m_memoryPropertyFlags;
	void* m_mapped = nullptr;

	Device* p_device;
public:
	void init(
		Device& device,
		VkBufferUsageFlags usageFlags,
		VkMemoryPropertyFlags memoryPropertyFlags,
		VkDeviceSize size,
		void* data
	);
	void cleanup();

	VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	void unmap();
	void writeTo(void* data, VkDeviceSize size);
	VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
	VkResult bind(VkDeviceSize offset = 0);

	inline VkBuffer getBuffer() { return m_buffer; }
	inline void* getMapped() { return m_mapped; }
	inline VkDeviceMemory getMemory() { return m_memory; }
	inline VkDeviceSize getSize() { return m_size; }

};

