#include "pch.h"
#include "Buffer.h"
#include "VkUtilities.h"
#include <assert.h>

void Buffer::init(
	Device& device, 
	VkBufferUsageFlags usageFlags,
	VkMemoryPropertyFlags memoryPropertyFlags,
	VkDeviceSize size, 
	void* data)
{
	p_device = &device;
	m_size = size;
	m_usageFlags = usageFlags;
	m_memoryPropertyFlags = memoryPropertyFlags;
	m_mapped = nullptr;

	VkBufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.usage = usageFlags;
	createInfo.size = size;
	if (vkCreateBuffer(device.getLogical(), &createInfo, nullptr, &m_buffer) != VK_SUCCESS)
		LOG_ERROR("Failed to create buffer!");

	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(device.getLogical(), m_buffer, &memReqs);

	VkMemoryAllocateInfo memAlloc{};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = VkUtils::findMemoryType(device, memReqs.memoryTypeBits, memoryPropertyFlags);

	VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
	if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
		allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
		allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
		memAlloc.pNext = &allocFlagsInfo;
	}
	if (vkAllocateMemory(device.getLogical(), &memAlloc, nullptr, &m_memory) != VK_SUCCESS)
		LOG_ERROR("Failed to allocate memory to buffer!");

	m_alignment = memReqs.size;

	if (data != nullptr)
	{
		if (map() != VK_SUCCESS)
			LOG_ERROR("Failed to map buffer!");
		memcpy(m_mapped, data, size);
		if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			flush();
		unmap();
	}
	if (bind() != VK_SUCCESS)
		LOG_ERROR("Failed to bind buffer to memory!");
}

void Buffer::cleanup()
{
	if (m_buffer)
	{
		vkDestroyBuffer(p_device->getLogical(), m_buffer, nullptr);
	}
	if (m_memory)
	{
		vkFreeMemory(p_device->getLogical(), m_memory, nullptr);
	}
}

VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset)
{
	return vkMapMemory(p_device->getLogical(), m_memory, offset, size, 0, &m_mapped);
}

void Buffer::unmap()
{
	if (m_mapped)
	{
		vkUnmapMemory(p_device->getLogical(), m_memory);
		m_mapped = nullptr;
	}
}

void Buffer::writeTo(void* data, VkDeviceSize size)
{
	assert(m_mapped);
	memcpy(m_mapped, data, size);
}

VkResult Buffer::flush(VkDeviceSize size, VkDeviceSize offset)
{
	VkMappedMemoryRange mappedRange = {};
	mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedRange.memory = m_memory;
	mappedRange.offset = offset;
	mappedRange.size = size;
	return vkFlushMappedMemoryRanges(p_device->getLogical(), 1, &mappedRange);
}

VkResult Buffer::bind(VkDeviceSize offset)
{
	return vkBindBufferMemory(p_device->getLogical(), m_buffer, m_memory, offset);
}
