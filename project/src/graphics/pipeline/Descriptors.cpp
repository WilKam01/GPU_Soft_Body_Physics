#include "pch.h"
#include "Descriptors.h"

void DescriptorSetLayout::init(Device& device, const std::vector<ShaderBinding>& shaderBindings)
{
    p_device = &device;
    m_bindings = shaderBindings;

    std::vector<VkDescriptorSetLayoutBinding> bindings(shaderBindings.size());
    for (int i = 0, len = (int)shaderBindings.size(); i < len; i++)
    {
        bindings[i].binding = shaderBindings[i].binding;
        bindings[i].descriptorType = shaderBindings[i].type;
        bindings[i].stageFlags = shaderBindings[i].shaderStage;
        bindings[i].descriptorCount = shaderBindings[i].count;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(p_device->getLogical(), &layoutInfo, nullptr, &m_layout) != VK_SUCCESS) {
        LOG_ERROR("Failed to create descriptor set layout!");
    }
}

void DescriptorSetLayout::cleanup()
{
    vkDestroyDescriptorSetLayout(p_device->getLogical(), m_layout, nullptr);
}

void DescriptorSetLayout::setPushConstant(uint32_t size, VkShaderStageFlagBits shaderStage)
{
    m_pushConstantSize = size;
    m_pushConstantShaderStage = shaderStage;
}

VkPushConstantRange DescriptorSetLayout::getPushConstantRange()
{
    VkPushConstantRange range{};
    range.offset = 0;
    range.size = m_pushConstantSize;
    range.stageFlags = m_pushConstantShaderStage;
    return range;
}

void DescriptorSet::createDescriptorPool()
{
    std::multiset<VkDescriptorType> poolSizeSet{};
    for (int i = 0, len = p_layout->bindingSize(); i < len; i++)
    {
        poolSizeSet.insert(p_layout->getBinding(i).type);
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

void DescriptorSet::init(Device& device, DescriptorSetLayout& layout, int count)
{
    p_device = &device;
    p_layout = &layout;
    m_descriptorSet.resize(count);

    createDescriptorPool();

    std::vector<VkDescriptorSetLayout> layouts(count, p_layout->get());
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
    descriptorWrites.descriptorType = p_layout->getBinding(binding).type;
    descriptorWrites.descriptorCount = 1;
    descriptorWrites.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(p_device->getLogical(), 1, &descriptorWrites, 0, nullptr);
}
