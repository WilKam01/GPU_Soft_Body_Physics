#pragma once

#include "graphics/Device.h"
#include "graphics/Buffer.h"
#include "resources/Texture.h"
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

	std::vector<VkDescriptorSetLayout> m_layout;
	std::vector<std::vector<ShaderBinding>> m_bindings;
public:
	// Vector of sets of vectors of bindings
	void init(Device& device, const std::vector<std::vector<ShaderBinding>>& shaderBindings);
	void cleanup();

	inline VkDescriptorSetLayout& get(size_t set) { return m_layout[set]; }
	inline std::vector<VkDescriptorSetLayout>& getAll() { return m_layout; }
	inline ShaderBinding getBinding(size_t set, size_t i) { return m_bindings[set][i]; }
	inline uint32_t bindingSize(size_t set) { return (uint32_t)m_bindings[set].size(); }
};

class DescriptorSet
{
private:
	Device* p_device;
	DescriptorSetLayout* p_layout;
	uint32_t m_set;

	std::vector<VkDescriptorSet> m_descriptorSet;
	VkDescriptorPool m_pool;

	void createDescriptorPool();
public:
	void init(Device& device, DescriptorSetLayout& layout, uint32_t set, uint32_t count = 1);
	void cleanup();

	void writeBuffer(size_t i, uint32_t binding, Buffer& buffer, VkDeviceSize range, VkDeviceSize offset = 0);
	void writeTexture(size_t i, uint32_t binding, Texture& texture, Sampler& sampler);

	VkDescriptorSet& get(size_t i) { return m_descriptorSet[i]; }
};

