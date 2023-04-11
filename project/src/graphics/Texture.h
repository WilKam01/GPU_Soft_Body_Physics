#pragma once

#include "Device.h"
#include "CommandPool.h"

class Texture
{
private:
	Device* p_device;

	VkImage m_image;
	VkImageView m_imageView;
	VkDeviceMemory m_memory;
	VkFormat m_format;
	glm::ivec2 m_dimensions;

	bool m_hasImageView;

	VkFormat findDepthFormat();

	void createImage(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
	void createImageView(VkImageAspectFlags aspectFlags);
public:

	void init(
		Device& device, 
		glm::ivec2 dimensions, 
		VkImageTiling tiling,
		VkImageUsageFlags usage, 
		VkMemoryPropertyFlags properties,
		void* data = nullptr
	);
	void initDepth(Device& device, glm::ivec2 dimensions);
	void cleanup();

	void transistionImageLayout(
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask,
		CommandPool& commandPool
	);
	static void transistionImageLayout(
		VkImage image,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask,
		CommandPool& commandPool
	);

	void exportJPG(const std::string& path);

	inline VkImage getImage() { return m_image; }
	inline VkImageView getView() { return m_imageView; }
	inline VkDeviceMemory getMemory() { return m_memory; }
	inline VkFormat getFormat() { return m_format; }
	inline glm::ivec2 getDimensions() { return m_dimensions; }
};

