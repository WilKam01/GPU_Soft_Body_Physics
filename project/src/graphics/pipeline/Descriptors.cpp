#include "pch.h"
#include "Descriptors.h"

void DescriptorSetLayout::init(Device& device, const std::vector<std::vector<ShaderBinding>>& shaderBindings)
{
    p_device = &device;
    m_bindings = shaderBindings;
    m_layout.resize(shaderBindings.size());

    std::vector<std::vector<VkDescriptorSetLayoutBinding>> bindings(shaderBindings.size());
    for (int i = 0, setLen = (int)shaderBindings.size(); i < setLen; i++)
    {
        bindings[i].resize(shaderBindings[i].size());

        for (int j = 0, bindLen = (int)shaderBindings[i].size(); j < bindLen; j++)
        {
            bindings[i][j].binding = shaderBindings[i][j].binding;
            bindings[i][j].descriptorType = shaderBindings[i][j].type;
            bindings[i][j].stageFlags = shaderBindings[i][j].shaderStage;
            bindings[i][j].descriptorCount = shaderBindings[i][j].count;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings[i].size());
        layoutInfo.pBindings = bindings[i].data();

        if (vkCreateDescriptorSetLayout(p_device->getLogical(), &layoutInfo, nullptr, &m_layout[i]) != VK_SUCCESS) {
            LOG_ERROR("Failed to create descriptor set layout!");
        }
    }
}

void DescriptorSetLayout::cleanup()
{
    for (int i = 0, len = (int)m_bindings.size(); i < len; i++)
        vkDestroyDescriptorSetLayout(p_device->getLogical(), m_layout[i], nullptr);
}

void DescriptorSet::createDescriptorPool()
{
    std::multiset<VkDescriptorType> poolSizeSet{};
    for (int i = 0, len = p_layout->bindingSize(m_set); i < len; i++)
    {
        poolSizeSet.insert(p_layout->getBinding(m_set, i).type);
    }

    std::vector<VkDescriptorPoolSize> poolSizes{};
    for (auto& set : poolSizeSet)
    {
        VkDescriptorPoolSize poolSize;
        poolSize.type = set;
        poolSize.descriptorCount = (uint32_t)(poolSizeSet.count(set) * m_descriptorSet.size());
        poolSizes.push_back(poolSize);
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(m_descriptorSet.size());

    if (vkCreateDescriptorPool(p_device->getLogical(), &poolInfo, nullptr, &m_pool) != VK_SUCCESS)
        LOG_ERROR("Failed to create descriptor pool!");
}

void DescriptorSet::init(Device& device, DescriptorSetLayout& layout, uint32_t set, uint32_t count)
{
    p_device = &device;
    p_layout = &layout;
    m_set = set;
    m_descriptorSet.resize(count);

    createDescriptorPool();

    std::vector<VkDescriptorSetLayout> layouts(count, p_layout->get(m_set));
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_pool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(count);
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(p_device->getLogical(), &allocInfo, m_descriptorSet.data()) != VK_SUCCESS)
        LOG_ERROR("Failed to allocate descriptor sets!");
}

void DescriptorSet::cleanup()
{
    vkDestroyDescriptorPool(p_device->getLogical(), m_pool, nullptr);
}

void DescriptorSet::writeBuffer(size_t i, uint32_t binding, Buffer& buffer, VkDeviceSize range, VkDeviceSize offset)
{
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.offset = offset;
    bufferInfo.buffer = buffer.get();
    bufferInfo.range = range;

    VkWriteDescriptorSet descriptorWrites{};
    descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites.dstSet = m_descriptorSet[i];
    descriptorWrites.dstBinding = binding;
    descriptorWrites.dstArrayElement = 0;
    descriptorWrites.descriptorType = p_layout->getBinding(m_set, binding).type;
    descriptorWrites.descriptorCount = 1;
    descriptorWrites.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(p_device->getLogical(), 1, &descriptorWrites, 0, nullptr);
}

void DescriptorSet::writeTexture(size_t i, uint32_t binding, Texture& texture, Sampler& sampler)
{
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture.getView();
    imageInfo.sampler = sampler.get();

    VkWriteDescriptorSet descriptorWrites{};
    descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites.dstSet = m_descriptorSet[i];
    descriptorWrites.dstBinding = binding;
    descriptorWrites.dstArrayElement = 0;
    descriptorWrites.descriptorType = p_layout->getBinding(m_set, binding).type;
    descriptorWrites.descriptorCount = 1;
    descriptorWrites.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(p_device->getLogical(), 1, &descriptorWrites, 0, nullptr);
}
