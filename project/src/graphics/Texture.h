#pragma once

#include "Device.h"

class Texture
{
private:
	Device* p_device;

	VkImage m_image;
	VkImageView m_imageView;
	VkDeviceMemory m_memory;
	VkFormat m_format;
	glm::ivec2 m_dimensions;

	VkFormat findDepthFormat();

	void createImage(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
	void createImageView(VkImageAspectFlags aspectFlags);
public:

	void initDepth(Device& device, glm::ivec2 dimensions);
	void cleanup();

	inline VkFormat getFormat() { return m_format; }
	inline VkImageView getView() { return m_imageView; }
};

