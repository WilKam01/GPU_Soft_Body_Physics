#pragma once

#include "../Device.h"
#include "../Buffer.h"
#include "../Texture.h"
#include "Sampler.h"

struct ShaderBinding
{
	uint32_t binding;
	VkDescriptorType type;
	VkShaderStageFlags shaderStage;
	uint32_t count = 1;
};

class DescriptorSetLayout
{
private:
	Device* p_device;

	VkDescriptorSetLayout m_layout;
	std::vector<ShaderBinding> m_bindings;
	uint32_t m_pushConstantSize;
	VkShaderStageFlagBits m_pushConstantShaderStage;
public:
	void init(Device& device, const std::vector<ShaderBinding>& shaderBindings);
	void cleanup();

	void setPushConstant(uint32_t size, VkShaderStageFlagBits shaderStage);

	inline VkDescriptorSetLayout& get() { return m_layout; }
	inline ShaderBinding getBinding(size_t i) { return m_bindings[i]; }
	inline uint32_t bindingSize() { return (uint32_t)m_bindings.size(); }
	VkPushConstantRange getPushConstantRange();
};

class DescriptorSet
{
private:
	Device* p_device;
	DescriptorSetLayout* p_layout;

	std::vector<VkDescriptorSet> m_descriptorSet;
	VkDescriptorPool m_pool;

	void createDescriptorPool();
public:
	void init(Device& device, DescriptorSetLayout& layout, int count);
	void cleanup();

	void writeBuffer(size_t i, uint32_t binding, Buffer& buffer, VkDeviceSize range, VkDeviceSize offset = 0);
	void writeTexture(size_t i, uint32_t binding, Texture& texture, Sampler& sampler);

	VkDescriptorSet& get(size_t i) { return m_descriptorSet[i]; }
};

