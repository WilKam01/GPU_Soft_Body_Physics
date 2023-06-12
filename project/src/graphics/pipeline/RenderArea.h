#pragma once

#include <vulkan/vulkan.h>

class RenderArea
{
private:
	VkViewport m_viewport;
	VkRect2D m_scissor;
public:
	void init(glm::uvec2 extent);
	void init(VkExtent2D extent);
	void bind(VkCommandBuffer commandBuffer);
};

