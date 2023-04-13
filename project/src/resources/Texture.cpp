#include "pch.h"
#include "Texture.h"
#include "graphics/CommandPool.h"

VkFormat Texture::findDepthFormat()
{
    std::vector<VkFormat> candidates{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(p_device->getPhysical(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            return format;
    }

    LOG_ERROR("Failed to find supported format!");
}

void Texture::createImage(VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_dimensions.x;
    imageInfo.extent.height = m_dimensions.y;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = m_format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT; //TODO: Msaa
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(p_device->getLogical(), &imageInfo, nullptr, &m_image) != VK_SUCCESS)
        LOG_ERROR("Failed to create image!");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(p_device->getLogical(), m_image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = p_device->findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(p_device->getLogical(), &allocInfo, nullptr, &m_memory) != VK_SUCCESS)
        LOG_ERROR("Failed to allocate image memory!");

    vkBindImageMemory(p_device->getLogical(), m_image, m_memory, 0);
}

void Texture::init(
    Device& device,
    CommandPool& commandPool,
    void* data,
    glm::uvec2 dimensions
)
{
    p_device = &device;
    m_dimensions = dimensions;
    m_format = VK_FORMAT_R8G8B8A8_UNORM;
    m_hasImageView = false;

    VkDeviceSize size = m_dimensions.x * m_dimensions.y * 4;

    Buffer stagingBuffer;
    stagingBuffer.init(device, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        size,
        data
    );

    createImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    transistionImageLayout(
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_ACCESS_NONE,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        commandPool
    );

    commandPool.copyBufferToImage(stagingBuffer, *this);

    transistionImageLayout(
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        commandPool
    );

    stagingBuffer.cleanup();

    createImageView(VK_IMAGE_ASPECT_COLOR_BIT);
}

void Texture::initEmpty(
    Device& device,
    glm::uvec2 dimensions,
    VkImageTiling tiling,
    VkImageUsageFlags usage, 
    VkMemoryPropertyFlags properties
)
{
    p_device = &device;
    m_dimensions = dimensions;
    m_format = VK_FORMAT_R8G8B8A8_UNORM;
    m_hasImageView = false;

    createImage(tiling, usage, properties);
}

void Texture::initDepth(Device& device, glm::uvec2 dimensions)
{
    p_device = &device;
    m_dimensions = dimensions;
    m_format = findDepthFormat();
    m_hasImageView = false;

    createImage(VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    createImageView(VK_IMAGE_ASPECT_DEPTH_BIT);
}

void Texture::cleanup()
{
    if(m_hasImageView)
        vkDestroyImageView(p_device->getLogical(), m_imageView, nullptr);
    vkDestroyImage(p_device->getLogical(), m_image, nullptr);
    vkFreeMemory(p_device->getLogical(), m_memory, nullptr);
}

void Texture::createImageView(VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(p_device->getLogical(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS)
        LOG_ERROR("Failed to create image view!");

    m_hasImageView = true;
}

void Texture::transistionImageLayout(
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    CommandPool& commandPool
)
{
    Texture::transistionImageLayout(m_image, m_format, oldLayout, newLayout, srcAccessMask, dstAccessMask, srcStageMask, dstStageMask, commandPool);
}

void Texture::transistionImageLayout(
    VkImage image,
    VkFormat format,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    CommandPool& commandPool
)
{
    VkCommandBuffer commandBuffer = commandPool.beginSingleTimeCommand();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        srcStageMask, dstStageMask,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    commandPool.endSingleTimeCommand(commandBuffer);
}
