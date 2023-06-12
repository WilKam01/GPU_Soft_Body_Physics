#include "pch.h"
#include "Sampler.h"

void Sampler::init(Device& device, VkSamplerAddressMode addressMode, VkSamplerMipmapMode mipmapMode)
{
    p_device = &device;

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(p_device->getPhysical(), &properties);

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = mipmapMode;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    samplerInfo.mipLodBias = 0.0f;

    if (vkCreateSampler(p_device->getLogical(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS)
        LOG_ERROR("Failed to create sampler!");
}

void Sampler::cleanup()
{
    vkDestroySampler(p_device->getLogical(), m_sampler, nullptr);
}
