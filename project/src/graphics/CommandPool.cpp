#include "pch.h"
#include "CommandPool.h"

void CommandPool::init(Device& device, VkPipelineBindPoint bindPoint)
{
    p_device = &device;
    m_bindPoint = bindPoint;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_bindPoint == VK_PIPELINE_BIND_POINT_COMPUTE ? 
        device.getQueueFamilyIndices().computeFamily.value() : device.getQueueFamilyIndices().graphicsFamily.value();

    if (vkCreateCommandPool(device.getLogical(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
        LOG_ERROR("Failed to create command pool!");
}

void CommandPool::cleanup()
{
    vkDestroyCommandPool(p_device->getLogical(), m_commandPool, nullptr);
}

VkCommandBuffer CommandPool::beginSingleTimeCommand()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(p_device->getLogical(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void CommandPool::endSingleTimeCommand(VkCommandBuffer buffer)
{
    vkEndCommandBuffer(buffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &buffer;

    if (m_bindPoint == VK_PIPELINE_BIND_POINT_COMPUTE)
    {
        vkQueueSubmit(p_device->getComputeQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(p_device->getComputeQueue());
    }
    else
    {
        vkQueueSubmit(p_device->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(p_device->getGraphicsQueue());
    }

    vkFreeCommandBuffers(p_device->getLogical(), m_commandPool, 1, &buffer);
}

void CommandPool::copyBuffer(Buffer& src, Buffer& dst, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommand();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, src.get(), dst.get(), 1, &copyRegion);

    endSingleTimeCommand(commandBuffer);
}
