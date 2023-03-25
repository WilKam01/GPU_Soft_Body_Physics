#include "pch.h"
#include "CommandBufferArray.h"

void CommandBufferArray::init(Device& device, CommandPool& commandPool, const uint32_t& size)
{
    p_device = &device;
    p_commandPool = &commandPool;
    m_commandBuffers.resize(size);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool.get();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = size;

    if (vkAllocateCommandBuffers(device.getLogical(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
        LOG_ERROR("Failed to allocate command buffers!");
}

void CommandBufferArray::cleanup()
{
    vkFreeCommandBuffers(p_device->getLogical(), p_commandPool->get(), (uint32_t)m_commandBuffers.size(), m_commandBuffers.data());
    m_commandBuffers.clear();
}

void CommandBufferArray::begin(size_t i)
{
    vkResetCommandBuffer(m_commandBuffers[i], 0);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo) != VK_SUCCESS)
        LOG_ERROR("Failed to begin recording command buffer!\nIndex: " + std::to_string(i));
}

void CommandBufferArray::end(size_t i)
{
    if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS)
        LOG_ERROR("Failed to record command buffer!");
}
