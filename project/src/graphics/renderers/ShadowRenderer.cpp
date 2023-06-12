#include "pch.h"
#include "ShadowRenderer.h"

void ShadowRenderer::createRenderPass()
{
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = m_depthTexture.getFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 0;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pColorAttachments = nullptr;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = nullptr;

    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &depthAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(p_device->getLogical(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
		LOG_ERROR("Failed to create shadow render pass.");
}

void ShadowRenderer::createFramebuffer()
{
    std::array<VkImageView, 1> attachments = {
        m_depthTexture.getView()
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = m_renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = m_size;
    framebufferInfo.height = m_size;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(p_device->getLogical(), &framebufferInfo, nullptr, &m_framebuffer) != VK_SUCCESS)
        LOG_ERROR("Failed to create shadow framebuffer!");
}

void ShadowRenderer::createPipeline()
{
    m_pipelineLayout.init(*p_device, nullptr, sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT);
    m_pipeline.initGraphics(
        *p_device,
        m_pipelineLayout,
        m_renderPass,
        "assets/spv/shadow.vert.spv",
        { VK_POLYGON_MODE_FILL, false, true },
        VERTEX_STREAM_INPUT_POSITION
    );
}

void ShadowRenderer::init(Device& device, SwapChain& swapChain, CommandPool& commandPool, uint32_t size)
{
	p_device = &device;
	p_swapChain = &swapChain;
	p_commandPool = &commandPool;
    m_size = size;

    m_renderArea.init(glm::uvec2(m_size));
    m_depthTexture.initDepth(device, glm::uvec2(m_size), VK_IMAGE_USAGE_SAMPLED_BIT);
    createRenderPass();
    createFramebuffer();
    createPipeline();
}

void ShadowRenderer::cleanup()
{
    m_pipeline.cleanup();
    m_pipelineLayout.cleanup();
    vkDestroyFramebuffer(p_device->getLogical(), m_framebuffer, nullptr);
    vkDestroyRenderPass(p_device->getLogical(), m_renderPass, nullptr);
    m_depthTexture.cleanup();
}

void ShadowRenderer::bind(VkCommandBuffer commandBuffer, glm::mat4 matrix)
{
    m_pipelineLayout.pushConstants(commandBuffer, sizeof(glm::mat4), &matrix);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_framebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = { m_size, m_size };

    VkClearValue clearValue{};
    clearValue.depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    m_renderArea.bind(commandBuffer);
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.get());
}
