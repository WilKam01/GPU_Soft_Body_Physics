#include "pch.h"
#include "ImGuiRenderer.h"
#include "core/Window.h"
#include "Renderer.h"

static void ImGuiVkResult(VkResult err)
{
    if (err == 0)
        return;

    LOG_ERROR("Vulkan error from ImGui: " + std::to_string(err));
}

void ImGuiRenderer::createRenderPass()
{
    // Color attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = p_swapChain->getFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // TODO: Msaa
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // Subpass dependency
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Render pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(p_device->getLogical(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
        LOG_ERROR("Failed to create ImGui Render pass.");
}

void ImGuiRenderer::createFramebuffers()
{
    int size = p_swapChain->getImageCount();
    m_framebuffers.resize(size);

    for (size_t i = 0; i < size; i++)
    {
        std::array<VkImageView, 1> attachments = {
            p_swapChain->getImageView(i)
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = p_swapChain->getExtent().width;
        framebufferInfo.height = p_swapChain->getExtent().height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(p_device->getLogical(), &framebufferInfo, nullptr, &m_framebuffers[i]) != VK_SUCCESS)
            LOG_ERROR("Failed to create ImGui framebuffer!");
    }
}

void ImGuiRenderer::init(Window& window, Instance& instance, Device& device, SwapChain& swapChain, CommandPool& commandPool)
{
    p_device = &device;
    p_swapChain = &swapChain;
    p_commandPool = &commandPool;

    createRenderPass();

    // Descriptor pool
    VkDescriptorPoolSize poolSizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 100 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100 }
    };

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 100;
    poolInfo.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]);
    poolInfo.pPoolSizes = poolSizes;

    if (vkCreateDescriptorPool(device.getLogical(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
        LOG_ERROR("Failed to create ImGui Descriptor pool!");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard |
                      ImGuiConfigFlags_NavEnableGamepad |
                      ImGuiConfigFlags_DockingEnable;
                      //ImGuiConfigFlags_ViewportsEnable; // Not working, fix later if needed

    io.ConfigWindowsResizeFromEdges = true;
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    for (uint32_t i = 0; i < ImGuiCol_COUNT; i++)
    {
        style.Colors[i] = ImVec4(
            std::pow(style.Colors[i].x, 2.2f),
            std::pow(style.Colors[i].y, 2.2f),
            std::pow(style.Colors[i].z, 2.2f),
            style.Colors[i].w
        );
    }

    window.initImGui();

    ImGui_ImplVulkan_InitInfo imguiInitInfo{};
    imguiInitInfo.Instance = instance.get();
    imguiInitInfo.PhysicalDevice = device.getPhysical();
    imguiInitInfo.Device = device.getLogical();
    imguiInitInfo.QueueFamily = device.getQueueFamilyIndices().graphicsFamily.value();
    imguiInitInfo.Queue = device.getGraphicsQueue();
    imguiInitInfo.PipelineCache = VK_NULL_HANDLE;
    imguiInitInfo.DescriptorPool = m_descriptorPool;
    imguiInitInfo.Subpass = 0;
    imguiInitInfo.MinImageCount = Renderer::MAX_FRAMES_IN_FLIGHT;
    imguiInitInfo.ImageCount = Renderer::MAX_FRAMES_IN_FLIGHT;
    imguiInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT; // TODO: Msaa
    imguiInitInfo.Allocator = nullptr;
    imguiInitInfo.CheckVkResultFn = ImGuiVkResult;
    ImGui_ImplVulkan_Init(&imguiInitInfo, m_renderPass);

    createFramebuffers();

    VkCommandBuffer commandBuffer = commandPool.beginSingleTimeCommand();
    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
    commandPool.endSingleTimeCommand(commandBuffer);

    device.waitIdle();
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void ImGuiRenderer::cleanup()
{
    for (auto framebuffer : m_framebuffers)
    {
        vkDestroyFramebuffer(p_device->getLogical(), framebuffer, nullptr);
    }

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyRenderPass(p_device->getLogical(), m_renderPass, nullptr);
    vkDestroyDescriptorPool(p_device->getLogical(), m_descriptorPool, nullptr);
}

void ImGuiRenderer::recreateFramebuffers()
{
    for (auto framebuffer : m_framebuffers)
    {
        vkDestroyFramebuffer(p_device->getLogical(), framebuffer, nullptr);
    }
    createFramebuffers();
}
