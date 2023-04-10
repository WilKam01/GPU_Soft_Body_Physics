#include "pch.h"
#include "Renderer.h"
#include "core/Window.h"
#include "VkUtilities.h"

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_swapChain.getRenderPass();
    renderPassInfo.framebuffer = m_swapChain.getFramebuffer(imageIndex);
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_swapChain.getExtent();

    VkClearValue clearColour { {0.0f, 0.0f, 0.0f, 1.0f} };

    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColour;

    vkCmdSetViewport(commandBuffer, 0, 1, &m_viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &m_scissor);

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline.get());

    VkBuffer vertexBuffers[] = { m_vertexBuffer.get() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer.get(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, 3, 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    m_imGuiRenderer.render();

    renderPassInfo.renderPass = m_imGuiRenderer.getRenderPass();
    renderPassInfo.framebuffer = m_imGuiRenderer.getFramebuffer(imageIndex);
    renderPassInfo.clearValueCount = 0;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
}

void Renderer::recordCommandBufferCompute(VkCommandBuffer commandBuffer)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline.get());
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout.get(), 0, 1, &m_computeDescriptorSet.get(currentFrame), 0, 0);

    vkCmdDispatch(commandBuffer, 3, 1, 1);
}

void Renderer::createSyncObjects()
{
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    m_computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(m_device.getLogical(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device.getLogical(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device.getLogical(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) 
            LOG_ERROR("Failed to create synchronization objects for a frame!");

        if (vkCreateSemaphore(m_device.getLogical(), &semaphoreInfo, nullptr, &m_computeFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device.getLogical(), &fenceInfo, nullptr, &m_computeInFlightFences[i]) != VK_SUCCESS)
            LOG_ERROR("Failed to create compute synchronization objects for a frame!");
    }
}

void Renderer::createMeshBuffers()
{
    static Vertex vertices[]
    {
        { glm::vec3(0.0f, 0.5f, 0.0f), 0.0f, glm::vec3(1.0f, 0.0f, 0.0f) },
        { glm::vec3(0.5f, -0.5f, 0.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f) },
        { glm::vec3(-0.5f, -0.5f, 0.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f) },
        //{ glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f) },
    };

    static uint32_t indices[]{ 1, 0, 2 };

    // Vertex buffer
    VkDeviceSize bufferSize = sizeof(vertices);
    Buffer stagingBuffer;
    stagingBuffer.init(m_device,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        bufferSize,
        vertices
    );

    m_vertexBuffer.init(m_device,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        bufferSize
    );

    m_commandPool.copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

    stagingBuffer.cleanup();

    // Index buffer
    bufferSize = sizeof(indices);
    stagingBuffer.init(m_device,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        bufferSize,
        indices
    );

    m_indexBuffer.init(m_device,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        bufferSize
    );

    m_commandPool.copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

    stagingBuffer.cleanup();
}

void Renderer::recreateSwapChain()
{
    m_swapChain.recreate();

    m_viewport.x = 0.0f;
    m_viewport.y = (float)m_swapChain.getExtent().height;
    m_viewport.width = (float)m_swapChain.getExtent().width;
    m_viewport.height = -((float)m_swapChain.getExtent().height);
    m_viewport.minDepth = 0.0f;
    m_viewport.maxDepth = 1.0f;
    
    m_scissor.offset = { 0, 0 };
    m_scissor.extent = m_swapChain.getExtent();

    m_imGuiRenderer.recreateFramebuffers();
}

void Renderer::init(Window& window)
{
	m_instance.init(window);
	window.createSurface(m_instance.get(), m_surface);
	m_device.init(m_instance, m_surface);
	m_swapChain.init(m_device, m_surface, window);

    m_viewport.x = 0.0f;
    m_viewport.y = (float)m_swapChain.getExtent().height;
    m_viewport.width = (float)m_swapChain.getExtent().width;
    m_viewport.height = -((float)m_swapChain.getExtent().height);
    m_viewport.minDepth = 0.0f;
    m_viewport.maxDepth = 1.0f;

    m_scissor.offset = { 0, 0 };
    m_scissor.extent = m_swapChain.getExtent();

    m_graphicsPipelineLayout.init(m_device);
    m_graphicsPipeline.initGraphics(m_device, m_graphicsPipelineLayout, nullptr, m_swapChain.getRenderPass(), "shaders/vert.spv", "shaders/frag.spv");

    m_computeDescriptorSetLayout.init(m_device,
    {
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
        { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT }
    });
    m_computeDescriptorSet.init(m_device, m_computeDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT);
    m_computePipelineLayout.init(m_device, &m_computeDescriptorSetLayout);
    m_computePipeline.initCompute(m_device, m_computePipelineLayout, &m_computeDescriptorSet, "shaders/comp.spv");

    VkDeviceSize bufferSize = sizeof(float);
    float dt = 0.0f;
    m_deltaTimeUBO.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_deltaTimeUBO[i].init(m_device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, bufferSize, &dt);
    }

    m_commandPool.init(m_device, VK_PIPELINE_BIND_POINT_GRAPHICS);
    m_commandBufferArray.init(m_device, m_commandPool, MAX_FRAMES_IN_FLIGHT);

    m_computeCommandPool.init(m_device, VK_PIPELINE_BIND_POINT_COMPUTE);
    m_computeCommandBufferArray.init(m_device, m_computeCommandPool, MAX_FRAMES_IN_FLIGHT);

    createSyncObjects();

    m_imGuiRenderer.init(window, m_instance, m_device, m_swapChain, m_commandPool, &m_vertexBuffer);

    createMeshBuffers();

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_computeDescriptorSet.writeBuffer(i, 0, m_deltaTimeUBO[i], bufferSize);
        m_computeDescriptorSet.writeBuffer(i, 1, m_vertexBuffer, m_vertexBuffer.getSize());
    }
}

void Renderer::cleanup()
{
    m_device.waitIdle();
    VkDevice device = m_device.getLogical();

    m_imGuiRenderer.cleanup();

    m_indexBuffer.cleanup();
    m_vertexBuffer.cleanup();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
    {
        vkDestroySemaphore(device, m_computeFinishedSemaphores[i], nullptr);
        vkDestroyFence(device, m_computeInFlightFences[i], nullptr);

        vkDestroySemaphore(device, m_renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, m_imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, m_inFlightFences[i], nullptr);
    }

    m_computeCommandBufferArray.cleanup();
    m_computeCommandPool.cleanup();

    m_commandBufferArray.cleanup();
    m_commandPool.cleanup();

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_deltaTimeUBO[i].cleanup();
    }

    m_computePipeline.cleanup();
    m_computePipelineLayout.cleanup();

    m_computeDescriptorSet.cleanup();
    m_computeDescriptorSetLayout.cleanup();

    m_graphicsPipeline.cleanup();
    m_graphicsPipelineLayout.cleanup();

	m_swapChain.cleanup();
	m_device.cleanup();
	vkDestroySurfaceKHR(m_instance.get(), m_surface, nullptr);
	m_instance.cleanup();
}

void Renderer::render()
{
    VkDevice device = m_device.getLogical();
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    vkWaitForFences(device, 1, &m_computeInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    vkResetFences(device, 1, &m_computeInFlightFences[currentFrame]);

    m_computeCommandBufferArray.begin(currentFrame);
    recordCommandBufferCompute(m_computeCommandBufferArray[currentFrame]);
    m_computeCommandBufferArray.end(currentFrame);

    float dt = ImGui::GetIO().DeltaTime;
    m_deltaTimeUBO[currentFrame].map();
    m_deltaTimeUBO[currentFrame].writeTo(&dt, sizeof(float));
    m_deltaTimeUBO[currentFrame].unmap();

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_computeCommandBufferArray[currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_computeFinishedSemaphores[currentFrame];

    if (vkQueueSubmit(m_device.getComputeQueue(), 1, &submitInfo, m_computeInFlightFences[currentFrame]) != VK_SUCCESS)
        LOG_ERROR("Failed to submit compute command buffer!");

    vkWaitForFences(device, 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, m_swapChain.get(), UINT64_MAX, m_imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        LOG_ERROR("Failed to acquire swap chain image!");

    vkResetFences(device, 1, &m_inFlightFences[currentFrame]);

    m_commandBufferArray.begin(currentFrame);
    recordCommandBuffer(m_commandBufferArray[currentFrame], imageIndex);
    m_commandBufferArray.end(currentFrame);

    // Update and Render additional Platform Windows
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    VkSemaphore waitSemaphores[] = { m_computeFinishedSemaphores[currentFrame], m_imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 2;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBufferArray[currentFrame];

    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_device.getGraphicsQueue(), 1, &submitInfo, m_inFlightFences[currentFrame]) != VK_SUCCESS)
        LOG_ERROR("Failed to submit draw command buffer!");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_swapChain.get() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    result = vkQueuePresentKHR(m_device.getPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_swapChain.needsRecreation())
        recreateSwapChain();
    else if (result != VK_SUCCESS)
        LOG_ERROR("Failed to present swap chain image!");

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
