#pragma once

#include "graphics/Device.h"

class Sampler
{
private:
	Device* p_device;

	VkSampler m_sampler;
public:
	void init(Device& device, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR);
	void cleanup();

	inline VkSampler get() { return m_sampler; }
};

