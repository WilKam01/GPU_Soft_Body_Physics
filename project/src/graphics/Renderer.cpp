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

    m_graphicsPipelineLayout.bindDescriptors(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, { m_graphicsDescriptorSet.get(currentFrame), m_meshDescriptorSet.get(0)});
    p_currentSoftBody->mesh.bind(commandBuffer);
    vkCmdDrawIndexed(commandBuffer, p_currentSoftBody->mesh.getIndexCount(), 1, 0, 0, 0);

    m_graphicsPipelineLayout.bindDescriptors(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, { m_graphicsDescriptorSet.get(currentFrame), m_floorDescriptorSet.get(0) });
    m_floorMesh.bind(commandBuffer);
    vkCmdDrawIndexed(commandBuffer, m_floorMesh.getIndexCount(), 1, 0, 0, 0);

    if (m_renderTetMesh)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_tetPipeline.get());
        m_tetPipelineLayout.bindDescriptors(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, { m_graphicsDescriptorSet.get(currentFrame), p_currentSoftBody->graphicsDescriptorSet.get(0) });
        vkCmdDraw(commandBuffer, 12, p_currentSoftBody->tetMesh.getTetCount(), 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);

    renderImGui();

    renderPassInfo.renderPass = m_imGuiRenderer.getRenderPass();
    renderPassInfo.framebuffer = m_imGuiRenderer.getFramebuffer(imageIndex);
    renderPassInfo.clearValueCount = 0;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
}

void Renderer::computePhysics(VkCommandBuffer commandBuffer)
{
    VkMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    m_pbdPipelineLayout.bindDescriptors(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, { m_pbdDescriptorSet.get(currentFrame), p_currentSoftBody->pbdDescriptorSet.get(0) });

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_presolvePipeline.get());
    vkCmdDispatch(commandBuffer, p_currentSoftBody->tetMesh.getParticleCount(), 1, 1);

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

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_distanceConstraintPipeline.get());
    vkCmdDispatch(commandBuffer, p_currentSoftBody->tetMesh.getEdgeCount(), 1, 1);

    /*vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        1,
        &memoryBarrier,
        0,
        nullptr,
        0,
        nullptr);*/

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_volumeConstraintPipeline.get());
    vkCmdDispatch(commandBuffer, p_currentSoftBody->tetMesh.getTetCount(), 1, 1);

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
    vkCmdDispatch(commandBuffer, p_currentSoftBody->tetMesh.getParticleCount(), 1, 1);

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
}

void Renderer::deformMesh(VkCommandBuffer commandBuffer)
{
    VkMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    m_deformPipelineLayout.bindDescriptors(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, { p_currentSoftBody->deformDescriptorSet.get(0) });

    if (p_currentSoftBody->useSkinning)
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_tetSkinningPipeline.get());
    else
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_deformPipeline.get());

    vkCmdDispatch(commandBuffer, p_currentSoftBody->mesh.getVertexCount(), 1, 1);

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

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_recalcNormalsPipeline.get());
    vkCmdDispatch(commandBuffer, p_currentSoftBody->mesh.getIndexCount() / 3, 1, 1);

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

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_normalizeNormalsPipeline.get());
    vkCmdDispatch(commandBuffer, p_currentSoftBody->mesh.getVertexCount(), 1, 1);

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
    m_softBody = createSoftBody("icosphere", glm::vec3(0.0f, 5.0f, 0.0f), 50);
    p_currentSoftBody = &m_softBody;

    static uint8_t pixel[4] = { 25, 25, 205, 255 };
    m_texture.init(m_device, m_commandPool, &pixel, glm::uvec2(1, 1));
    m_sampler.init(m_device);

    static float scale = 200.0f;
    static float uvScale = 50.0f;
    static VertexStream floorVertices
    {
        {
            { glm::vec3(1.0f, 0.0f, 1.0f) * scale },
            { glm::vec3(-1.0f, 0.0f, -1.0f) * scale },
            { glm::vec3(-1.0f, 0.0f, 1.0f) * scale },
            { glm::vec3(1.0f, 0.0f, -1.0f) * scale }
        },
        {
            { glm::vec3(0.0f, 1.0f, 0.0f) },
            { glm::vec3(0.0f, 1.0f, 0.0f) },
            { glm::vec3(0.0f, 1.0f, 0.0f) },
            { glm::vec3(0.0f, 1.0f, 0.0f) }
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

    MeshData floorMeshData = { floorVertices, floorIndices };
    m_floorMesh.init(m_device, m_commandPool, floorMeshData);
    m_floorTexture = ResourceManager::loadTexture("assets/textures/check.jpg");
}

SoftBody Renderer::createSoftBody(const std::string& name, glm::vec3 offset, int resolution)
{
    SoftBody softBody;
    MeshData mesh = ResourceManager::loadMeshOBJ("assets/models/" + name + ".obj", offset);
    std::string resStr = resolution == 100 ? "" : std::to_string(resolution);
    TetrahedralMeshData tetMesh = ResourceManager::loadTetrahedralMeshOBJ("assets/tet_models/" + name + resStr + ".obj", offset);

    if (!mesh.vertices.positions.size() || !tetMesh.particles.size())
    {
        softBody.active = false;
        return softBody;
    }

    softBody.mesh.init(m_device, m_commandPool, mesh);
    softBody.tetMesh.init(m_device, m_commandPool, tetMesh);

    softBody.graphicsDescriptorSet.init(m_device, m_tetDescriptorSetLayout, 1);
    softBody.graphicsDescriptorSet.writeBuffer(0, 0, softBody.tetMesh.getParticleBuffer());
    softBody.graphicsDescriptorSet.writeBuffer(0, 1, softBody.tetMesh.getTetBuffer());

    softBody.pbdDescriptorSet.init(m_device, m_pbdDescriptorSetLayout, 1);
    softBody.pbdDescriptorSet.writeBuffer(0, 0, softBody.tetMesh.getParticleBuffer());
    softBody.pbdDescriptorSet.writeBuffer(0, 1, softBody.tetMesh.getPbdPosBuffer());
    softBody.pbdDescriptorSet.writeBuffer(0, 2, softBody.tetMesh.getEdgeBuffer());
    softBody.pbdDescriptorSet.writeBuffer(0, 3, softBody.tetMesh.getTetBuffer());

    softBody.deformDescriptorSet.init(m_device, m_deformDescriptorSetLayout, 0);
    softBody.deformDescriptorSet.writeBuffer(0, 0, softBody.mesh.getVertexBuffer(0));
    softBody.deformDescriptorSet.writeBuffer(0, 1, softBody.mesh.getVertexBuffer(1));
    softBody.deformDescriptorSet.writeBuffer(0, 2, softBody.mesh.getIndexBuffer());
    softBody.deformDescriptorSet.writeBuffer(0, 3, softBody.tetMesh.getPbdPosBuffer());
    softBody.deformDescriptorSet.writeBuffer(0, 5, softBody.tetMesh.getTetBuffer());

    Buffer stagingBuffer;
    VkDeviceSize bufferSize = 0;
    // No skinning
    if (resolution == 100)
    {
        bufferSize = sizeof(uint32_t) * softBody.mesh.getVertexCount();
        stagingBuffer.init(m_device,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            bufferSize,
            (void*)mesh.origIndices.data()
        );
        softBody.useSkinning = false;
    }
    // Tetrahedral skinning
    else
    {
        std::vector<SkinningInfo> skinningInfo(mesh.vertices.positions.size());

        // Tetrahedral info
        int tetCount = (int)tetMesh.tets.size();
        std::vector<glm::vec4> tetCenters(tetCount); // w component is max radius
        std::vector<glm::mat3> tetMatrices(tetCount);

        for (int i = 0; i < tetCount; i++)
        {
            glm::uvec4 indices = tetMesh.tets[i].indices;
            glm::vec3 tetCenter =
                (tetMesh.particles[indices[0]].position +
                tetMesh.particles[indices[1]].position +
                tetMesh.particles[indices[2]].position +
                tetMesh.particles[indices[3]].position) * 0.25f;
            
            float maxRadius = 0.0f;
            for (int j = 0; j < 4; j++)
            {
                glm::vec3 diff = tetMesh.particles[indices[j]].position - tetCenter;
                maxRadius = std::max(maxRadius, glm::dot(diff, diff));
            }

            tetCenters[i] = glm::vec4(tetCenter.x, tetCenter.y, tetCenter.z, maxRadius + 0.025f);
            tetMatrices[i] = glm::inverse(
                glm::mat3(
                    tetMesh.particles[indices[0]].position - tetMesh.particles[indices[3]].position,
                    tetMesh.particles[indices[1]].position - tetMesh.particles[indices[3]].position,
                    tetMesh.particles[indices[2]].position - tetMesh.particles[indices[3]].position
                )
            );
        }

        // Apply to mesh vertices
        for (int i = 0, len = (int)mesh.vertices.positions.size(); i < len; i++)
        {
            float minDist = FLT_MAX;
            for (int j = 0; j < tetCount; j++)
            {
                glm::vec3 diff = mesh.vertices.positions[i].vec - glm::vec3(tetCenters[j].x, tetCenters[j].y, tetCenters[j].z);
                float dist = glm::dot(diff, diff);
                if (dist > tetCenters[j].w)
                    continue;

                diff = mesh.vertices.positions[i].vec - tetMesh.particles[tetMesh.tets[j].indices[3]].position;
                diff = tetMatrices[j] * diff;

                float baryCoords[4]{ diff.x, diff.y, diff.z, 1.0f - (diff.x + diff.y + diff.z) };
                float maxDist = 0.0f;
                for (int k = 0; k < 4; k++)
                    maxDist = std::max(maxDist, -baryCoords[k]);

                if (maxDist < minDist)
                {
                    minDist = maxDist;
                    skinningInfo[i].tetId = j;
                    skinningInfo[i].weights = glm::vec3(baryCoords[0], baryCoords[1], baryCoords[2]);

                    // Surrounded by tetrhedral
                    if (maxDist <= 0.0f)
                        break;
                }
            }
        }

        bufferSize = sizeof(SkinningInfo) * (uint32_t)skinningInfo.size();
        stagingBuffer.init(m_device,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            bufferSize,
            (void*)skinningInfo.data()
        );
        softBody.useSkinning = true;
    }

    softBody.deformBuffer.init(m_device,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        bufferSize
    );

    m_commandPool.copyBuffer(stagingBuffer, softBody.deformBuffer, bufferSize);
    stagingBuffer.cleanup();

    softBody.deformDescriptorSet.writeBuffer(0, 4, softBody.deformBuffer);
    softBody.active = true;

    return softBody;
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
        m_graphicsUBO[i].get().viewProj = m_camera.getMatrix();
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

    static PbdUBO& pbd = m_pbdUBO[currentFrame].get();

    ImGui::Begin("Physics Settings");

    ImGui::SliderInt("Fixed time step (fps)", &m_fixedTimeStep, 10, 300);
    ImGui::SliderInt("Substep count", &m_subSteps, 1, 25);
    ImGui::SliderFloat("Edge compliance", &pbd.edgeCompliance, 0.0f, 1.0f);
    ImGui::SliderFloat("Volume compliance", &pbd.volumeCompliance, 0.0f, 1.0f);
    ImGui::Checkbox("Render wireframe", &m_renderTetMesh);
    if (ImGui::Button("Restart"))
        p_currentSoftBody->tetMesh.reset();

    float timeStep = 1.0f / (float)m_fixedTimeStep;
    m_timer.setFixedDT(timeStep);
    pbd.deltaTime = timeStep / m_subSteps;

    m_pbdUBO[currentFrame].get() = pbd;
    m_pbdUBO[currentFrame].update();

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
    static char name[25];
    static int resolution = 100;
    static glm::vec3 offset = glm::vec3(0.0f, 5.0f, 0.0f);

    ImGui::Begin("Scene settings");

    ImGui::SliderFloat4("Global ambient", (float*)&ubo.globalAmbient, 0.0f, 1.0f);
    ImGui::SliderFloat3("Light position", (float*)&ubo.lightPos, -10.0f, 10.0f);
    ImGui::SliderFloat("Light intensity", &ubo.lightIntensity, -1.0f, 10.0f);
    ImGui::SliderFloat("Light cone", &ubo.lightCone, 0.0f, 1.0f);
    ImGui::SliderFloat("Specular power", &ubo.specPower, 0.0f, 100.0f);

    ImGui::Text("");
    ImGui::Text("Camera settings");
    glm::vec3 camPos = m_camera.getPosition(), camRot = m_camera.getRotation();
    ImGui::SliderFloat3("Position", (float*)&camPos, -10.0f, 10.0f, "%.4f");
    ImGui::SliderFloat3("Rotation", (float*)&camRot, -180.0f, 180.0f, "%.4f");

    ImGui::Text("");
    ImGui::Text("Soft body model");
    ImGui::InputText("Name", name, 100);
    ImGui::SliderInt("Resolution", &resolution, 1, 100);
    ImGui::SliderFloat3("Start offset", (float*)&offset, 0.0f, 10.0f);
    if (ImGui::Button("Load"))
    {
        std::ifstream stream;
        stream.open("assets/models/" + std::string(name) + ".obj");
        if (stream.is_open())
        {
            if (p_currentSoftBody == &m_softBody)
            {
                m_softBody1.cleanup();
                m_softBody1 = createSoftBody(name, offset, resolution);
                if(m_softBody1.active)
                    p_currentSoftBody = &m_softBody1;
            }
            else
            {
                m_softBody.cleanup();
                m_softBody = createSoftBody(name, offset, resolution);
                if (m_softBody.active)
                    p_currentSoftBody = &m_softBody;
            }
            m_timer.reset();
        }
        stream.close();
    }

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
            { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },
            { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT }
        }
    });
    m_tetPipelineLayout.init(m_device, &m_tetDescriptorSetLayout);
    m_tetPipeline.initGraphics(
        m_device,
        m_tetPipelineLayout,
        m_swapChain.getRenderPass(),
        "assets/spv/tetrahedral.vert.spv",
        "assets/spv/simple.frag.spv",
        { VK_POLYGON_MODE_LINE, false, false},
        VERTEX_STREAM_INPUT_NONE
    );

    m_pbdDescriptorSetLayout.init(m_device,
    {
        {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
        },
        {
            { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT }
        }
    });
    m_pbdDescriptorSet.init(m_device, m_pbdDescriptorSetLayout, 0, MAX_FRAMES_IN_FLIGHT);
    m_pbdPipelineLayout.init(m_device, &m_pbdDescriptorSetLayout);
    m_presolvePipeline.initCompute(m_device, m_pbdPipelineLayout, "assets/spv/presolve.comp.spv");
    m_distanceConstraintPipeline.initCompute(m_device, m_pbdPipelineLayout, "assets/spv/distance_constraint.comp.spv");
    m_volumeConstraintPipeline.initCompute(m_device, m_pbdPipelineLayout, "assets/spv/volume_constraint.comp.spv");
    m_postsolvePipeline.initCompute(m_device, m_pbdPipelineLayout, "assets/spv/postsolve.comp.spv");

    m_deformDescriptorSetLayout.init(m_device,
    {
        {
            { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT }
        }
    });
    m_deformPipelineLayout.init(m_device, &m_deformDescriptorSetLayout);
    m_deformPipeline.initCompute(m_device, m_deformPipelineLayout, "assets/spv/deform.comp.spv");
    m_tetSkinningPipeline.initCompute(m_device, m_deformPipelineLayout, "assets/spv/tetrahedral_skinning.comp.spv");
    m_recalcNormalsPipeline.initCompute(m_device, m_deformPipelineLayout, "assets/spv/recalculate_normals.comp.spv");
    m_normalizeNormalsPipeline.initCompute(m_device, m_deformPipelineLayout, "assets/spv/normalize_normals.comp.spv");

    m_graphicsUBO.resize(MAX_FRAMES_IN_FLIGHT);
    m_pbdUBO.resize(MAX_FRAMES_IN_FLIGHT);
    float dt = (1.0f / (float)m_fixedTimeStep) / m_subSteps;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_graphicsUBO[i].init(m_device, graphics);
        m_pbdUBO[i].init(m_device, { dt, 0.01f, 0.01f });
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

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_graphicsDescriptorSet.writeBuffer(i, 0, m_graphicsUBO[i]);
        m_pbdDescriptorSet.writeBuffer(i, 0, m_pbdUBO[i]);
    }

    m_timer.init(1.0f / m_fixedTimeStep);
}

void Renderer::cleanup()
{
    m_device.waitIdle();
    VkDevice device = m_device.getLogical();

    m_imGuiRenderer.cleanup();

    m_softBody.cleanup();
    m_softBody1.cleanup();

    m_floorTexture.cleanup();
    m_floorMesh.cleanup();

    m_sampler.cleanup();
    m_texture.cleanup();

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
        m_pbdUBO[i].cleanup();
        m_graphicsUBO[i].cleanup();
    }

    m_normalizeNormalsPipeline.cleanup();
    m_recalcNormalsPipeline.cleanup();
    m_tetSkinningPipeline.cleanup();
    m_deformPipeline.cleanup();
    m_deformPipelineLayout.cleanup();
    m_deformDescriptorSetLayout.cleanup();

    m_postsolvePipeline.cleanup();
    m_volumeConstraintPipeline.cleanup();
    m_distanceConstraintPipeline.cleanup();
    m_presolvePipeline.cleanup();
    m_pbdPipelineLayout.cleanup();

    m_pbdDescriptorSet.cleanup();
    m_pbdDescriptorSetLayout.cleanup();

    m_tetPipeline.cleanup();
    m_tetPipelineLayout.cleanup();

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
    if (m_timer.passedFixedDT())
    {
        for(int i = 0; i < m_subSteps; i++)
            computePhysics(m_computeCommandBufferArray[currentFrame]);

        deformMesh(m_computeCommandBufferArray[currentFrame]);
    }
     m_computeCommandBufferArray.end(currentFrame);

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
