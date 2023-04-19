#include "pch.h"
#include "Renderer.h"
#include "core/Window.h"

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_swapChain.getRenderPass();
    renderPassInfo.framebuffer = m_swapChain.getFramebuffer(imageIndex);
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_swapChain.getExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdSetViewport(commandBuffer, 0, 1, &m_viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &m_scissor);

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline.get());

    /*m_graphicsPipelineLayout.bindDescriptors(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, { m_graphicsDescriptorSet.get(currentFrame), m_meshDescriptorSet.get(0)});
    m_mesh.bind(commandBuffer);
    vkCmdDrawIndexed(commandBuffer, m_mesh.getIndexCount(), 1, 0, 0, 0);*/

    m_graphicsPipelineLayout.bindDescriptors(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, { m_graphicsDescriptorSet.get(currentFrame), m_floorDescriptorSet.get(0) });
    m_floorMesh.bind(commandBuffer);
    vkCmdDrawIndexed(commandBuffer, m_floorMesh.getIndexCount(), 1, 0, 0, 0);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_tetPipeline.get());
    m_tetPipelineLayout.bindDescriptors(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, { m_graphicsDescriptorSet.get(currentFrame), m_meshDescriptorSet.get(0), m_tetDescriptorSet.get(0) });
    vkCmdDraw(commandBuffer, 12, m_tetMesh.getTetCount(), 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    renderImGui();

    renderPassInfo.renderPass = m_imGuiRenderer.getRenderPass();
    renderPassInfo.framebuffer = m_imGuiRenderer.getFramebuffer(imageIndex);
    renderPassInfo.clearValueCount = 0;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
}

void Renderer::recordCommandBufferCompute(VkCommandBuffer commandBuffer)
{
    VkMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    m_pbdPipelineLayout.bindDescriptors(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, { m_pbdDescriptorSet.get(currentFrame) });

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_presolvePipeline.get());
    vkCmdDispatch(commandBuffer, m_tetMesh.getParticleCount(), 1, 1);

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        1,
        &memoryBarrier,
        0,
        nullptr,
        0,
        nullptr);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_postsolvePipeline.get());
    vkCmdDispatch(commandBuffer, m_tetMesh.getParticleCount(), 1, 1);
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

void Renderer::createResources()
{
    MeshData mesh = ResourceManager::loadMeshOBJ("assets/models/bunny.obj", glm::vec3(0.0f, 1.0f, 0.0f));
    m_mesh.init(m_device, m_commandPool, mesh);

    TetrahedralMeshData tetMesh = ResourceManager::loadTetrahedralMeshOBJ("assets/tet_models/bunnyTet.obj", glm::vec3(0.0f, 5.0f, 0.0f));
    /*TetrahedralMeshData tetMesh = {
        {
            { glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) },
            { glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f) },
            { glm::vec4(-1.0f, 1.0f, 1.0f, 1.0f) },
            { glm::vec4(1.0f, 1.0f, -1.0f, 1.0f) },
            { glm::vec4(1.0f, -1.0f, 1.0f, 1.0f) },
            { glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f) },
            { glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f) },
            { glm::vec4(1.0f, -1.0f, -1.0f, 1.0f) },
            { glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) }
        },
        {
            glm::uvec4(0, 3, 4, 8),
            glm::uvec4(7, 4, 3, 8),
            glm::uvec4(0, 2, 3, 8),
            glm::uvec4(1, 3, 2, 8),
            glm::uvec4(1, 5, 7, 8),
            glm::uvec4(1, 7, 3, 8),
            glm::uvec4(1, 2, 5, 8),
            glm::uvec4(2, 6, 5, 8),
            glm::uvec4(0, 4, 2, 8),
            glm::uvec4(2, 4, 6, 8),
            glm::uvec4(4, 7, 6, 8),
            glm::uvec4(5, 6, 7, 8)
        }
    };*/
    m_tetMesh.init(m_device, m_commandPool, tetMesh);

    static uint8_t pixel[4] = { 25, 25, 205, 255 };
    m_texture.init(m_device, m_commandPool, &pixel, glm::uvec2(1, 1));
    m_sampler.init(m_device);

    static float scale = 200.0f;
    static float uvScale = 50.0f;
    static VertexStream floorVertices
    {
        {
            glm::vec3(1.0f, 0.0f, 1.0f) * scale,
            glm::vec3(-1.0f, 0.0f, -1.0f) * scale,
            glm::vec3(-1.0f, 0.0f, 1.0f) * scale,
            glm::vec3(1.0f, 0.0f, -1.0f) * scale
        },
        {
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        },
        {
            glm::vec2(1.0f, 0.0f) * uvScale,
            glm::vec2(0.0f, 1.0f) * uvScale,
            glm::vec2(0.0f, 0.0f) * uvScale,
            glm::vec2(1.0f, 1.0f) * uvScale,
        }
    };

    static std::vector<uint32_t> floorIndices
    {
        0, 1, 2,
        0, 3, 1,
    };

    m_floorMesh.init(m_device, m_commandPool, { floorVertices, floorIndices });
    m_floorTexture = ResourceManager::loadTexture("assets/textures/check.jpg");
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

    m_camera.updateAspectRatio(m_swapChain.getExtent().width / (float)m_swapChain.getExtent().height);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        GraphicsUBO& graphics = m_graphicsUBO[i].get();
        graphics.viewProj = m_camera.getMatrix();
        m_graphicsUBO[i].update();
    }

    m_imGuiRenderer.recreateFramebuffers();

    m_timer.reset();
}

void Renderer::renderImGui()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    //------------------------------------------

    ImGui::Begin("Stats");

    ImGui::Text("runtime: %.3f s", m_timer.getTotal());
    ImGui::Text("delta: %.4f ms", m_timer.getDT());
    ImGui::Text("FPS: %.3f", 1.0f / m_timer.getDT());

    ImGui::End();

    ImGui::Begin("Copy image");

    if (ImGui::Button("Copy!"))
    {
        Texture texture = m_swapChain.copyImage(currentFrame, m_commandPool);
        std::ifstream stream;
        for (int i = 0; i < 100; i++)
        {
            stream.open("../screenshots/" + std::to_string(i) + ".jpg");
            if (stream.is_open())
                stream.close();
            else
            {
                ResourceManager::exportJPG(texture, "../screenshots/" + std::to_string(i) + ".jpg");
                break;
            }
        }
        texture.cleanup();
    }

    ImGui::End();

    static GraphicsUBO& ubo = m_graphicsUBO[currentFrame].get();
    
    ImGui::Begin("Scene settings");

    ImGui::SliderFloat4("Global ambient", (float*)&ubo.globalAmbient, 0.0f, 1.0f);
    ImGui::SliderFloat3("Light position", (float*)&ubo.lightPos, -10.0f, 10.0f);
    ImGui::SliderFloat("Light intensity", &ubo.lightIntensity, -1.0f, 10.0f);
    ImGui::SliderFloat("Light cone", &ubo.lightCone, 0.0f, 1.0f);
    ImGui::SliderFloat("Specular power", &ubo.specPower, 0.0f, 100.0f);

    ImGui::Text("Camera settings");
    glm::vec3 camPos = m_camera.getPosition(), camRot = m_camera.getRotation();
    ImGui::SliderFloat3("Position", (float*)&camPos, -10.0f, 10.0f, "%.4f");
    ImGui::SliderFloat3("Rotation", (float*)&camRot, -180.0f, 180.0f, "%.4f");

    m_camera.setPosition(camPos);
    m_camera.setRotation(camRot);
    ubo.camPos = camPos;
    ubo.viewProj = m_camera.getMatrix();

    m_graphicsUBO[currentFrame].get() = ubo;
    m_graphicsUBO[currentFrame].update();

    ImGui::End();

    //------------------------------------------

    ImGui::Render();
}

void Renderer::init(Window& window)
{
	m_instance.init(window);
	window.createSurface(m_instance.get(), m_surface);
	m_device.init(m_instance, m_surface);
	m_swapChain.init(m_device, m_surface, window);

    m_camera.init(glm::vec3(0.0f, 5.0f, 6.0f), glm::vec3(-30.0f, 0.0f, 0.0f), 90.0f, m_swapChain.getExtent().width / (float)m_swapChain.getExtent().height);

    currentFrame = 0;
    GraphicsUBO graphics{};
    graphics.viewProj = m_camera.getMatrix();
    graphics.globalAmbient = glm::vec4(0.01f);
    graphics.lightPos = glm::vec3(0.0f, 10.0f, 5.0f);
    graphics.lightDir = glm::normalize(-graphics.lightPos);
    graphics.lightCone = 0.75f;
    graphics.lightIntensity = 3.0f;
    graphics.specPower = 70.0f;
    graphics.camPos = m_camera.getPosition();

    m_viewport.x = 0.0f;
    m_viewport.y = (float)m_swapChain.getExtent().height;
    m_viewport.width = (float)m_swapChain.getExtent().width;
    m_viewport.height = -((float)m_swapChain.getExtent().height);
    m_viewport.minDepth = 0.0f;
    m_viewport.maxDepth = 1.0f;

    m_scissor.offset = { 0, 0 };
    m_scissor.extent = m_swapChain.getExtent();

    m_graphicsDescriptorSetLayout.init(m_device,
    {
        {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }
        },
        {
            { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }
        }
    });
    m_graphicsDescriptorSet.init(m_device, m_graphicsDescriptorSetLayout, 0, MAX_FRAMES_IN_FLIGHT);
    m_meshDescriptorSet.init(m_device, m_graphicsDescriptorSetLayout, 1);
    m_floorDescriptorSet.init(m_device, m_graphicsDescriptorSetLayout, 1);

    m_graphicsPipelineLayout.init(m_device, &m_graphicsDescriptorSetLayout);
    m_graphicsPipeline.initGraphics(
        m_device, 
        m_graphicsPipelineLayout, 
        m_swapChain.getRenderPass(), 
        "assets/spv/shader.vert.spv", 
        "assets/spv/shader.frag.spv",
        { VK_POLYGON_MODE_FILL, true, true },
        VERTEX_STREAM_INPUT_ALL
    );

    m_tetDescriptorSetLayout.init(m_device,
    {
        {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }
        },
        {
            { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }
        },
        {
            { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },
            { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT }
        }
    });
    m_tetDescriptorSet.init(m_device, m_tetDescriptorSetLayout, 2);
    m_tetPipelineLayout.init(m_device, &m_tetDescriptorSetLayout);
    m_tetPipeline.initGraphics(
        m_device,
        m_tetPipelineLayout,
        m_swapChain.getRenderPass(),
        "assets/spv/tetrahedral.vert.spv",
        "assets/spv/shader.frag.spv",
        { VK_POLYGON_MODE_LINE, true, true},
        VERTEX_STREAM_INPUT_NONE
    );

    m_pbdDescriptorSetLayout.init(m_device,
    {
        {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT }
        }
    });
    m_pbdDescriptorSet.init(m_device, m_pbdDescriptorSetLayout, 0, MAX_FRAMES_IN_FLIGHT);
    m_pbdPipelineLayout.init(m_device, &m_pbdDescriptorSetLayout);
    m_presolvePipeline.initCompute(m_device, m_pbdPipelineLayout, "assets/spv/presolve.comp.spv");
    m_postsolvePipeline.initCompute(m_device, m_pbdPipelineLayout, "assets/spv/postsolve.comp.spv");

    m_graphicsUBO.resize(MAX_FRAMES_IN_FLIGHT);
    m_deltaTimeUBO.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_graphicsUBO[i].init(m_device, graphics);
        m_deltaTimeUBO[i].init(m_device, 0.0f);
    }

    m_commandPool.init(m_device, VK_PIPELINE_BIND_POINT_GRAPHICS);
    m_commandBufferArray.init(m_device, m_commandPool, MAX_FRAMES_IN_FLIGHT);

    m_computeCommandPool.init(m_device, VK_PIPELINE_BIND_POINT_COMPUTE);
    m_computeCommandBufferArray.init(m_device, m_computeCommandPool, MAX_FRAMES_IN_FLIGHT);

    createSyncObjects();

    m_imGuiRenderer.init(window, m_instance, m_device, m_swapChain, m_commandPool);
    ResourceManager::init(m_device, m_commandPool);

    createResources();

    m_meshDescriptorSet.writeTexture(0, 0, m_texture, m_sampler);
    m_floorDescriptorSet.writeTexture(0, 0, m_floorTexture, m_sampler);

    m_tetDescriptorSet.writeBuffer(0, 0, m_tetMesh.getParticleBuffer(), m_tetMesh.getParticleBuffer().getSize());
    m_tetDescriptorSet.writeBuffer(0, 1, m_tetMesh.getTetIdBuffer(), m_tetMesh.getTetIdBuffer().getSize());

    m_pbdDescriptorSet.writeBuffer(0, 1, m_tetMesh.getParticleBuffer(), m_tetMesh.getParticleBuffer().getSize());
    m_pbdDescriptorSet.writeBuffer(0, 2, m_tetMesh.getPredictPosBuffer(), m_tetMesh.getPredictPosBuffer().getSize());

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_graphicsDescriptorSet.writeBuffer(i, 0, m_graphicsUBO[i], m_graphicsUBO[i].size());

        m_pbdDescriptorSet.writeBuffer(i, 0, m_deltaTimeUBO[i], m_deltaTimeUBO[i].size());
        m_pbdDescriptorSet.writeBuffer(i, 1, m_tetMesh.getParticleBuffer(), m_tetMesh.getParticleBuffer().getSize());
        m_pbdDescriptorSet.writeBuffer(i, 2, m_tetMesh.getPredictPosBuffer(), m_tetMesh.getPredictPosBuffer().getSize());
    }

    m_timer.init();
}

void Renderer::cleanup()
{
    m_device.waitIdle();
    VkDevice device = m_device.getLogical();

    m_imGuiRenderer.cleanup();

    m_floorTexture.cleanup();
    m_floorMesh.cleanup();

    m_sampler.cleanup();
    m_texture.cleanup();
    m_mesh.cleanup();
    m_tetMesh.cleanup();

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
        m_graphicsUBO[i].cleanup();
        m_deltaTimeUBO[i].cleanup();
    }

    m_postsolvePipeline.cleanup();
    m_presolvePipeline.cleanup();
    m_pbdPipelineLayout.cleanup();

    m_pbdDescriptorSet.cleanup();
    m_pbdDescriptorSetLayout.cleanup();

    m_tetPipeline.cleanup();
    m_tetPipelineLayout.cleanup();

    m_tetDescriptorSet.cleanup();
    m_tetDescriptorSetLayout.cleanup();

    m_graphicsPipeline.cleanup();
    m_graphicsPipelineLayout.cleanup();

    m_floorDescriptorSet.cleanup();
    m_meshDescriptorSet.cleanup();
    m_graphicsDescriptorSet.cleanup();
    m_graphicsDescriptorSetLayout.cleanup();

	m_swapChain.cleanup();
	m_device.cleanup();
	vkDestroySurfaceKHR(m_instance.get(), m_surface, nullptr);
	m_instance.cleanup();
}

void Renderer::render()
{
    m_timer.update();

    VkDevice device = m_device.getLogical();
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    vkWaitForFences(device, 1, &m_computeInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    vkResetFences(device, 1, &m_computeInFlightFences[currentFrame]);

    m_computeCommandBufferArray.begin(currentFrame);
    recordCommandBufferCompute(m_computeCommandBufferArray[currentFrame]);
    m_computeCommandBufferArray.end(currentFrame);

    m_deltaTimeUBO[currentFrame].get() = m_timer.getDT();
    m_deltaTimeUBO[currentFrame].update();

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

    //m_camera.setRotation(m_camera.getRotation() + glm::vec3(0.0f, m_timer.getDT() * 75.0f, 0.0f));
    //m_graphicsUBO[currentFrame].get().viewProj = m_camera.getMatrix();
    //m_graphicsUBO[currentFrame].update();

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
