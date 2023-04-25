#include "pch.h"
#include "Pipeline.h"

void PipelineLayout::init(Device& device, DescriptorSetLayout* layout)
{
    p_device = &device;
    p_descriptorSetLayout = layout;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (layout != nullptr)
    {
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(p_descriptorSetLayout->getAll().size());
        pipelineLayoutInfo.pSetLayouts = p_descriptorSetLayout->getAll().data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
    }

    if (vkCreatePipelineLayout(p_device->getLogical(), &pipelineLayoutInfo, nullptr, &m_layout) != VK_SUCCESS)
        LOG_ERROR("Failed to create pipeline layout!");
}

void PipelineLayout::cleanup()
{
    vkDestroyPipelineLayout(p_device->getLogical(), m_layout, nullptr);
}

void PipelineLayout::bindDescriptors(VkCommandBuffer commandBuffer, VkPipelineBindPoint bindPoint, const std::vector<VkDescriptorSet>& descriptorSets)
{
    vkCmdBindDescriptorSets(
        commandBuffer,
        bindPoint,
        m_layout,
        0,
        static_cast<uint32_t>(descriptorSets.size()),
        descriptorSets.data(),
        0, 
        0
    );
}

VkShaderModule Pipeline::loadShader(const std::string& path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        LOG_ERROR("Failed to open file!");

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = buffer.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(p_device->getLogical(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        LOG_ERROR("Failed to create shader module!");

    return shaderModule;
}

void Pipeline::initGraphics(
    Device& device,
    PipelineLayout& layout,
    VkRenderPass renderPass,
    const std::string& vertexShaderPath,
    const std::string& fragmentShaderPath,
    PipelineSettings settings,
    VertexStreamInput inputStreams
)
{
    p_device = &device;
    p_layout = &layout;
    m_bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    VkShaderModule vertShaderModule = loadShader(vertexShaderPath);
    VkShaderModule fragShaderModule = loadShader(fragmentShaderPath);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    std::vector<VkVertexInputBindingDescription> vertexInputBindingDescs{};
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescs{};

    if (inputStreams & VERTEX_STREAM_INPUT_POSITION)
    {
        vertexInputBindingDescs.push_back({ (uint32_t)vertexInputBindingDescs.size(), sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX });
        uint32_t index = (uint32_t)vertexInputAttributeDescs.size();
        vertexInputAttributeDescs.push_back({ index, index, VK_FORMAT_R32G32B32_SFLOAT, 0 });
    }
    if (inputStreams & VERTEX_STREAM_INPUT_NORMAL)
    {
        vertexInputBindingDescs.push_back({ (uint32_t)vertexInputBindingDescs.size(), sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX });
        uint32_t index = (uint32_t)vertexInputAttributeDescs.size();
        vertexInputAttributeDescs.push_back({ index, index, VK_FORMAT_R32G32B32_SFLOAT, 0 });
    }
    if (inputStreams & VERTEX_STREAM_INPUT_UV)
    {
        vertexInputBindingDescs.push_back({ (uint32_t)vertexInputBindingDescs.size(), sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX });
        uint32_t index = (uint32_t)vertexInputAttributeDescs.size();
        vertexInputAttributeDescs.push_back({ index, index, VK_FORMAT_R32G32_SFLOAT, 0 });
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)vertexInputBindingDescs.size();
    vertexInputInfo.pVertexBindingDescriptions = vertexInputBindingDescs.data();
    vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)vertexInputAttributeDescs.size();
    vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributeDescs.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = settings.polygonMode;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = settings.backfaceCulling ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // TODO: Msaa
    multisampling.sampleShadingEnable = VK_TRUE;
    multisampling.minSampleShading = .2f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = (uint32_t)(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = settings.depthTesting ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = p_layout->get();
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(p_device->getLogical(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
        LOG_ERROR("Failed to create pipeline!");

    vkDestroyShaderModule(p_device->getLogical(), fragShaderModule, nullptr);
    vkDestroyShaderModule(p_device->getLogical(), vertShaderModule, nullptr);
}

void Pipeline::initCompute(
    Device& device,
    PipelineLayout& layout,
    const std::string& shaderPath
)
{
    p_device = &device;
    p_layout = &layout;
    m_bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

    VkShaderModule shaderModule = loadShader(shaderPath);

    VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = shaderModule;
    computeShaderStageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = p_layout->get();
    pipelineInfo.stage = computeShaderStageInfo;

    if (vkCreateComputePipelines(p_device->getLogical(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
        LOG_ERROR("Failed to create pipeline!");

    vkDestroyShaderModule(p_device->getLogical(), shaderModule, nullptr);
}

void Pipeline::cleanup()
{
    vkDestroyPipeline(p_device->getLogical(), m_pipeline, nullptr);
}
