#pragma once

#include "../Device.h"

class Sampler
{
private:
	Device* p_device;

	VkSampler m_sampler;
public:
	void init(Device& device);
	void cleanup();

	inline VkSampler get() { return m_sampler; }
};

