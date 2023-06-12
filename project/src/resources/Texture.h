#pragma once

#include "graphics/Device.h"

class CommandPool;

class Texture
{
private:
	Device* p_device;

	VkImage m_image;
	VkImageView m_imageView;
	VkDeviceMemory m_memory;
	VkFormat m_format;
	glm::uvec2 m_dimensions;

	bool m_hasImageView;
	bool m_isDepth;

	VkFormat findDepthFormat();

	void createImage(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
public:

	// Data is expected to be of rgba format
	void init(
		Device& device,
		CommandPool& commandPool,
		void* data,
		glm::uvec2 dimensions
	);
	void initEmpty(
		Device& device, 
		glm::uvec2 dimensions, 
		VkImageTiling tiling,
		VkImageUsageFlags usage, 
		VkMemoryPropertyFlags properties
	);
	void initDepth(Device& device, glm::uvec2 dimensions, VkImageUsageFlagBits additionalUsage = (VkImageUsageFlagBits)0);
	void cleanup();

	void createImageView(VkImageAspectFlags aspectFlags);
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

	inline VkImage getImage() { return m_image; }
	inline VkImageView getView() { return m_imageView; }
	inline VkDeviceMemory getMemory() { return m_memory; }
	inline VkFormat getFormat() { return m_format; }
	inline glm::uvec2 getDimensions() { return m_dimensions; }
	inline bool isDepth() { return m_isDepth; }
};

