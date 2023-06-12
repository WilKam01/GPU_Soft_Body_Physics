#include "pch.h"
#include "RenderArea.h"

void RenderArea::init(glm::uvec2 extent)
{
    m_viewport.x = 0.0f;
    m_viewport.y = (float)extent.y;
    m_viewport.width = (float)extent.x;
    m_viewport.height = -((float)extent.y);
    m_viewport.minDepth = 0.0f;
    m_viewport.maxDepth = 1.0f;

    m_scissor.offset = { 0, 0 };
    m_scissor.extent = { extent.x, extent.y };
}

void RenderArea::init(VkExtent2D extent)
{
    m_viewport.x = 0.0f;
    m_viewport.y = (float)extent.height;
    m_viewport.width = (float)extent.width;
    m_viewport.height = -((float)extent.height);
    m_viewport.minDepth = 0.0f;
    m_viewport.maxDepth = 1.0f;

    m_scissor.offset = { 0, 0 };
    m_scissor.extent = extent;
}

void RenderArea::bind(VkCommandBuffer commandBuffer)
{
    vkCmdSetViewport(commandBuffer, 0, 1, &m_viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &m_scissor);
}
