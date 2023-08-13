#include "pch.h"
#include "Renderer.h"
#include "core/Window.h"

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    static float orthoSize = 15.0f;
    static float dist = 15.0f;

    glm::mat4 lightMatrix = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 100.0f);
    lightMatrix *= glm::lookAt(-m_graphicsUBO[currentFrame].get().lightDir * dist, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    m_matricesUBO[currentFrame].get().viewProj = m_camera.getMatrix();
    m_matricesUBO[currentFrame].get().light = lightMatrix;
    m_matricesUBO[currentFrame].update();

    m_shadowRenderer.bind(commandBuffer, lightMatrix);

    for (auto& softBody : m_softBodies)
    {
        if (!softBody.active)
            break;

        softBody.mesh.bind(commandBuffer);
        vkCmdDrawIndexed(commandBuffer, softBody.mesh.getIndexCount(), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_swapChain.getRenderPass();
    renderPassInfo.framebuffer = m_swapChain.getFramebuffer(imageIndex);
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_swapChain.getExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    m_renderArea.bind(commandBuffer);
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline.get());

    m_graphicsPipelineLayout.bindDescriptors(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, { m_graphicsDescriptorSet.get(currentFrame), m_meshDescriptorSet.get(0)});
    for (auto& softBody : m_softBodies)
    {
        if (!softBody.active)
            break;
        
        m_material.tint = softBody.color;
        softBody.mesh.bind(commandBuffer);
        m_graphicsPipelineLayout.pushConstants(commandBuffer, sizeof(Material), &m_material);
        vkCmdDrawIndexed(commandBuffer, softBody.mesh.getIndexCount(), 1, 0, 0, 0);
    }

    m_graphicsPipelineLayout.bindDescriptors(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, { m_graphicsDescriptorSet.get(currentFrame), m_floorDescriptorSet.get(0) });
    m_graphicsPipelineLayout.pushConstants(commandBuffer, sizeof(Material), &m_floorMaterial);
    m_floorMesh.bind(commandBuffer);
    vkCmdDrawIndexed(commandBuffer, m_floorMesh.getIndexCount(), 1, 0, 0, 0);

    if (m_renderTetMesh)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_tetPipeline.get());
        for (auto& softBody : m_softBodies)
        {
            if (!softBody.active)
                break;

            m_tetPipelineLayout.bindDescriptors(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, { m_graphicsDescriptorSet.get(currentFrame), softBody.graphicsDescriptorSet.get(0) });
            vkCmdDraw(commandBuffer, 12, softBody.tetMesh.getTetCount(), 0, 0);
        }
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

void Renderer::detectCollisions(VkCommandBuffer commandBuffer, SoftBody& softBody)
{
    static uint32_t zero = 0;
    softBody.colSizeBuffer[currentFrame].map();
    softBody.colSizeBuffer[currentFrame].writeTo(&zero, sizeof(uint32_t));
    softBody.colSizeBuffer[currentFrame].unmap();

    m_colPipelineLayout.bindDescriptors(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, { m_colDescriptorSet.get(currentFrame), softBody.colDescriptorSet.get(currentFrame) });

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_staticColDetectionPipeline.get());
    vkCmdDispatch(commandBuffer, (m_colUBO[currentFrame].get().triCount + 31) / 32, 1, 1);
}

void Renderer::computePhysics(VkCommandBuffer commandBuffer, SoftBody& softBody)
{
    VkMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    m_pbdPipelineLayout.bindDescriptors(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, { m_pbdDescriptorSet.get(currentFrame), softBody.pbdDescriptorSet.get(0) });

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_presolvePipeline.get());
    vkCmdDispatch(commandBuffer, (softBody.tetMesh.getParticleCount() + 31) / 32, 1, 1);

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

    m_colPipelineLayout.bindDescriptors(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, { m_colDescriptorSet.get(currentFrame), softBody.colDescriptorSet.get(currentFrame) });

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_colConstraintPipeline.get());
    vkCmdDispatch(commandBuffer, (MAX_COLLISION_CONSTRAINT_COUNT + 31) / 32, 1, 1);

    m_pbdPipelineLayout.bindDescriptors(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, { m_pbdDescriptorSet.get(currentFrame), softBody.pbdDescriptorSet.get(0) });
    
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_stretchConstraintPipeline.get());
    vkCmdDispatch(commandBuffer, (softBody.tetMesh.getEdgeCount() + 31) / 32, 1, 1);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_volumeConstraintPipeline.get());
    vkCmdDispatch(commandBuffer, (softBody.tetMesh.getTetCount() + 31) / 32, 1, 1);

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
    vkCmdDispatch(commandBuffer, (softBody.tetMesh.getParticleCount() + 31) / 32, 1, 1);

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

void Renderer::deformMesh(VkCommandBuffer commandBuffer, SoftBody& softBody)
{
    VkMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    m_deformPipelineLayout.bindDescriptors(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, { softBody.deformDescriptorSet.get(0) });

    if (softBody.useTetDeformation)
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_tetDeformPipeline.get());
    else
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_deformPipeline.get());

    vkCmdDispatch(commandBuffer, (softBody.mesh.getVertexCount() + 31) / 32, 1, 1);

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
    vkCmdDispatch(commandBuffer, (softBody.mesh.getIndexCount() / 3 + 31) / 32, 1, 1);

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
    vkCmdDispatch(commandBuffer, (softBody.mesh.getVertexCount() + 31) / 32, 1, 1);

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
    m_softBodies[0] = createSoftBody(m_modelName, m_offset);

    m_texture = m_resources.loadTexture("assets/textures/texture.jpg");
    m_sampler.init(m_device);
    m_material.roughness = 0.5f;
    m_material.metallic = 0.0f;

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
    m_floorMesh.init(m_device, m_commandPool, &floorMeshData);
    m_floorTexture = m_resources.loadTexture("assets/textures/check.jpg");

    m_floorMaterial.tint = glm::vec3(1.0f);
    m_floorMaterial.roughness = 1.0f;
    m_floorMaterial.metallic = 0.0f;

    // Collision data
    VkDeviceSize bufferSize = sizeof(avec3) * floorVertices.positions.size();
    Buffer stagingBuffer;
    stagingBuffer.init(m_device,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        bufferSize,
        (void*)floorVertices.positions.data()
    );

    m_colPositionsBuffer.init(m_device,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        bufferSize
    );

    m_commandPool.copyBuffer(stagingBuffer, m_colPositionsBuffer, bufferSize);
    stagingBuffer.cleanup();

    bufferSize = sizeof(uint32_t) * floorIndices.size();
    stagingBuffer.init(m_device,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        bufferSize,
        (void*)floorIndices.data()
    );

    m_colIndicesBuffer.init(m_device,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        bufferSize
    );

    m_commandPool.copyBuffer(stagingBuffer, m_colIndicesBuffer, bufferSize);
    stagingBuffer.cleanup();

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_colUBO[i].get().triCount = (uint32_t)floorIndices.size() / 3;
        m_colUBO[i].update();
    }
}

SoftBody Renderer::createSoftBody(const std::string& name, glm::vec3 offset, int resolution)
{
    SoftBody softBody;
    SoftBodyData* softBodyData = m_resources.getSoftBody(name, resolution);

    if (!softBodyData)
        return softBody;

    softBody.mesh.init(m_device, m_commandPool, softBodyData->mesh);
    softBody.tetMesh.init(m_device, m_commandPool, &softBodyData->tetMesh, offset);

    softBody.pbdUBO.init(m_device, glm::uvec3(softBody.tetMesh.getParticleCount(), softBody.tetMesh.getEdgeCount(), softBody.tetMesh.getTetCount()));
    softBody.deformUBO.init(m_device, glm::uvec2(softBody.mesh.getVertexCount(), softBody.mesh.getIndexCount()));

    softBody.graphicsDescriptorSet.init(m_device, m_tetDescriptorSetLayout, 1);
    softBody.graphicsDescriptorSet.writeBuffer(0, 0, softBody.tetMesh.getParticleBuffer());
    softBody.graphicsDescriptorSet.writeBuffer(0, 1, softBody.tetMesh.getTetBuffer());

    softBody.pbdDescriptorSet.init(m_device, m_pbdDescriptorSetLayout, 1);
    softBody.pbdDescriptorSet.writeBuffer(0, 0, softBody.pbdUBO);
    softBody.pbdDescriptorSet.writeBuffer(0, 1, softBody.tetMesh.getParticleBuffer());
    softBody.pbdDescriptorSet.writeBuffer(0, 2, softBody.tetMesh.getPbdPosBuffer());
    softBody.pbdDescriptorSet.writeBuffer(0, 3, softBody.tetMesh.getEdgeBuffer());
    softBody.pbdDescriptorSet.writeBuffer(0, 4, softBody.tetMesh.getTetBuffer());

    softBody.deformDescriptorSet.init(m_device, m_deformDescriptorSetLayout, 0);
    softBody.deformDescriptorSet.writeBuffer(0, 0, softBody.deformUBO);
    softBody.deformDescriptorSet.writeBuffer(0, 1, softBody.mesh.getVertexBuffer(0));
    softBody.deformDescriptorSet.writeBuffer(0, 2, softBody.mesh.getVertexBuffer(1));
    softBody.deformDescriptorSet.writeBuffer(0, 3, softBody.mesh.getIndexBuffer());
    softBody.deformDescriptorSet.writeBuffer(0, 4, softBody.tetMesh.getPbdPosBuffer());
    softBody.deformDescriptorSet.writeBuffer(0, 6, softBody.tetMesh.getTetBuffer());

    softBody.colDescriptorSet.init(m_device, m_colDescriptorSetLayout, 1, MAX_FRAMES_IN_FLIGHT);
    softBody.colSizeBuffer.resize(MAX_FRAMES_IN_FLIGHT);
    softBody.colConstraintBuffer.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        softBody.colSizeBuffer[i].init(m_device,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            sizeof(uint32_t),
            0
        );

        softBody.colConstraintBuffer[i].init(m_device,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            sizeof(ColConstraint) * MAX_COLLISION_CONSTRAINT_COUNT,
            0
        );

        softBody.colDescriptorSet.writeBuffer(i, 0, softBody.pbdUBO);
        softBody.colDescriptorSet.writeBuffer(i, 1, softBody.tetMesh.getParticleBuffer());
        softBody.colDescriptorSet.writeBuffer(i, 2, softBody.tetMesh.getPbdPosBuffer());
        softBody.colDescriptorSet.writeBuffer(i, 3, softBody.colSizeBuffer[i]);
        softBody.colDescriptorSet.writeBuffer(i, 4, softBody.colConstraintBuffer[i]);
    }

    Buffer stagingBuffer;
    VkDeviceSize bufferSize = 0;

    // No tetrahedral deformation
    if (resolution == 100)
    {
        bufferSize = sizeof(uint32_t) * softBody.mesh.getVertexCount();
        stagingBuffer.init(m_device,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            bufferSize,
            (void*)softBodyData->mesh->origIndices.data()
        );
        softBody.useTetDeformation = false;
    }
    // Tetrahedral deformation
    else
    {
        bufferSize = sizeof(DeformationInfo) * (uint32_t)softBodyData->deformationInfo.size();
        stagingBuffer.init(m_device,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            bufferSize,
            (void*)softBodyData->deformationInfo.data()
        );
        softBody.useTetDeformation = true;
    }

    softBody.deformBuffer.init(m_device,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        bufferSize
    );

    m_commandPool.copyBuffer(stagingBuffer, softBody.deformBuffer, bufferSize);
    stagingBuffer.cleanup();

    softBody.deformDescriptorSet.writeBuffer(0, 5, softBody.deformBuffer);

    softBody.color = COLORS[rand() % COLOR_COUNT];
    softBody.active = true;

    return softBody;
}

void Renderer::recreateSwapChain()
{
    m_swapChain.recreate();

    m_renderArea.init(m_swapChain.getExtent());
    m_camera.updateAspectRatio(m_swapChain.getExtent().width / (float)m_swapChain.getExtent().height);
    m_imGuiRenderer.recreateFramebuffers();

    m_timer.reset();
}

void Renderer::renderImGui()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    glm::vec3 moveDir(
        (float)ImGui::IsKeyDown(ImGuiKey_D) - (float)ImGui::IsKeyDown(ImGuiKey_A), 
        (float)ImGui::IsKeyDown(ImGuiKey_Q) - (float)ImGui::IsKeyDown(ImGuiKey_E),
        (float)ImGui::IsKeyDown(ImGuiKey_S) - (float)ImGui::IsKeyDown(ImGuiKey_W)
    );
    if(moveDir != glm::vec3(0.0f))
        moveDir = glm::normalize(moveDir);

    glm::vec3 rotDir(
        (float)ImGui::IsKeyDown(ImGuiKey_I) - (float)ImGui::IsKeyDown(ImGuiKey_K),
        (float)ImGui::IsKeyDown(ImGuiKey_J) - (float)ImGui::IsKeyDown(ImGuiKey_L),
        0.0f
    );
    if (rotDir != glm::vec3(0.0f))
        rotDir = glm::normalize(rotDir);

    static GraphicsUBO& ubo = m_graphicsUBO[currentFrame].get();
    static bool takeInput = false;

    //------------------------------------------

    if (m_renderImGui)
    {
        ImGui::Begin("Stats");

        float averageDT = m_timer.getAverageDT();
        ImGui::Text("runtime: %.3f s", m_timer.getTotal());
        ImGui::Text("average delta: %.4f ms", averageDT * 1000.0f);
        ImGui::Text("average FPS: %.3f", 1.0f / averageDT);

        ImGui::End();

        static PbdUBO& pbd = m_pbdUBO[currentFrame].get();

        ImGui::Begin("Physics Settings");

        ImGui::SliderInt("Fixed time step (fps)", &m_fixedTimeStep, 10, 240);
        ImGui::SliderInt("Substep count", &m_subSteps, 1, 25);
        ImGui::SliderFloat("Edge compliance", &pbd.edgeCompliance, 0.0f, 1.0f);
        ImGui::SliderFloat("Volume compliance", &pbd.volumeCompliance, 0.0f, 1.0f);
        ImGui::Checkbox("Render wireframe", &m_renderTetMesh);

        float timeStep = 1.0f / (float)m_fixedTimeStep;
        m_timer.setFixedDT(timeStep);
        pbd.deltaTime = timeStep / m_subSteps;

        m_pbdUBO[currentFrame].get() = pbd;
        m_pbdUBO[currentFrame].update();

        m_colUBO[currentFrame].get().deltaTime = timeStep;
        m_colUBO[currentFrame].update();

        ImGui::End();

        ImGui::Begin("Scene settings");

        static glm::vec3 rot = glm::vec3(45.0f, 0.0f, -45.0f);
        ImGui::SliderFloat3("Light rotation", (float*)&rot, -180.0f, 180.0f);
        ImGui::SliderFloat("Light intensity", &ubo.lightIntensity, 0.0f, 10.0f);
        ImGui::SliderFloat("Ambient", &ubo.ambient, 0.0f, 1.0f);
        ImGui::SliderFloat("Fresnel", &ubo.fresnel, 0.0f, 1.0f);
        ImGui::SliderFloat("Shadow aplha", &ubo.shadowAlpha, 0.0f, 1.0f);
        ubo.lightDir = glm::rotate(glm::mat4(1.0f), glm::radians(rot.z), glm::vec3(0.0f, 0.0f, 1.0f)) *
                       glm::rotate(glm::mat4(1.0f), glm::radians(rot.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
                       glm::rotate(glm::mat4(1.0f), glm::radians(rot.x), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);

        ImGui::Text("Floor Material");
        ImGui::PushID(0);
        ImGui::SliderFloat("Roughness", &m_floorMaterial.roughness, 0.0f, 1.0f);
        ImGui::SliderFloat("Metallic", &m_floorMaterial.metallic, 0.0f, 1.0f);
        ImGui::PopID();

        ImGui::Text("Main Material");
        ImGui::SliderFloat("Roughness", &m_material.roughness, 0.0f, 1.0f);
        ImGui::SliderFloat("Metallic", &m_material.metallic, 0.0f, 1.0f);

        ImGui::Text("Soft body model");
        ImGui::InputText("Name", m_modelName, 25);
        takeInput = !ImGui::IsItemActive();
        ImGui::SliderInt("Resolution", &m_modelResolution, 1, 100);
        ImGui::SliderFloat3("Start offset", (float*)&m_offset, 0.0f, 10.0f);
        ImGui::SliderInt("Number of bodies", &m_modelCount, 1, MAX_SOFT_BODY_COUNT);
        ImGui::SliderInt("Frame measure count", &m_frameCount, 100, MAX_FRAME_MEASUREMENT_COUNT);

        if (ImGui::Button("Load"))
        {
            for (int i = 0; i < MAX_SOFT_BODY_COUNT; i++)
            {
                if (!m_softBodies[i].active)
                    break;
                m_removeBodies.push_back(&m_softBodies[i]);
            }

            m_loadSoftBodies = 1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Measure FPS") && m_measureFrameCounter == MAX_FRAME_MEASUREMENT_COUNT)
        {
            m_avgFPS = std::vector<float>(m_frameCount, 0.0f);
            m_measureFPS = true;
            m_measureFrameCounter = 0;

            for (int i = 0; i < MAX_SOFT_BODY_COUNT; i++)
            {
                if (!m_softBodies[i].active)
                    break;
                m_removeBodies.push_back(&m_softBodies[i]);
            }

            m_loadSoftBodies = 1;
        }
        ImGui::SameLine();
        if (ImGui::Button("Measure Error") && m_measureFrameCounter == MAX_FRAME_MEASUREMENT_COUNT)
        {
            m_avgError = std::vector<float>(m_frameCount, 0.0f);
            m_avgCenterError = std::vector<float>(m_frameCount, 0.0f);
            m_avgCenterPos[0] = std::vector<glm::vec3>(m_frameCount, glm::vec3(0.0f));
            m_avgCenterPos[1] = std::vector<glm::vec3>(m_frameCount, glm::vec3(0.0f));
            m_measureFPS = false;
            m_measureFrameCounter = 0;
            m_warmupCounter = 0;

            for (int i = 0; i < MAX_SOFT_BODY_COUNT; i++)
            {
                if (!m_softBodies[i].active)
                    break;
                m_removeBodies.push_back(&m_softBodies[i]);
            }

            m_loadSoftBodies = 2;
        }
        ImGui::End();
    }

    //------------------------------------------

    if (ImGui::IsKeyPressed(ImGuiKey_H) && takeInput)
        m_renderImGui = !m_renderImGui;

    if (takeInput)
    {
        glm::vec3 camPos = m_camera.getPosition();
        glm::vec3 camRot = m_camera.getRotation();

        camRot += rotDir * m_timer.getDT() * 100.0f;
        m_camera.setRotation(camRot);

        camPos += m_camera.toLocal(moveDir) * m_timer.getDT() * 10.0f;
        m_camera.setPosition(camPos);
        ubo.camPos = camPos;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_P) && takeInput)
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
                m_resources.exportJPG(texture, "../screenshots/" + std::to_string(i) + ".jpg");
                stream.close();
                break;
            }
        }
        texture.cleanup();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_R) && takeInput)
    {
        for (int i = 0; i < MAX_SOFT_BODY_COUNT; i++)
        {
            if (!m_softBodies[i].active)
                break;
            m_removeBodies.push_back(&m_softBodies[i]);
        }

        m_loadSoftBodies = 1;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Z) && takeInput)
        m_renderTetMesh = !m_renderTetMesh;

    m_graphicsUBO[currentFrame].get() = ubo;
    m_graphicsUBO[currentFrame].update();

    ImGui::Render();
}

void Renderer::init(Window& window)
{
	m_instance.init(window);
	window.createSurface(m_instance.get(), m_surface);
	m_device.init(m_instance, m_surface);
	m_swapChain.init(m_device, m_surface, window);

    m_camera.init(
        glm::vec3(0.0f, 5.0f, 6.0f),
        glm::vec3(-30.0f, 0.0f, 0.0f),
        90.0f,
        m_swapChain.getExtent().width / (float)m_swapChain.getExtent().height
    );

    currentFrame = 0;
    GraphicsUBO graphics{};
    graphics.camPos = m_camera.getPosition();
    graphics.lightDir = glm::vec3(0.0f);
    graphics.lightIntensity = 2.5f;
    graphics.ambient = 0.05f;
    graphics.fresnel = 0.04f;
    graphics.shadowAlpha = 0.35f;

    m_renderArea.init(m_swapChain.getExtent());
    m_graphicsDescriptorSetLayout.init(m_device,
    {
        {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },
            { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT },
            { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }
        },
        {
            { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }
        }
    });
    m_graphicsDescriptorSet.init(m_device, m_graphicsDescriptorSetLayout, 0, MAX_FRAMES_IN_FLIGHT);
    m_meshDescriptorSet.init(m_device, m_graphicsDescriptorSetLayout, 1);
    m_floorDescriptorSet.init(m_device, m_graphicsDescriptorSetLayout, 1);

    m_graphicsPipelineLayout.init(m_device, &m_graphicsDescriptorSetLayout, sizeof(Material));
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
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },
            { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT },
            { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }
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
        { VK_POLYGON_MODE_LINE, false, false },
        VERTEX_STREAM_INPUT_NONE
    );

    m_pbdDescriptorSetLayout.init(m_device,
    {
        {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
        },
        {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT }
        }
    });
    m_pbdDescriptorSet.init(m_device, m_pbdDescriptorSetLayout, 0, MAX_FRAMES_IN_FLIGHT);
    m_pbdPipelineLayout.init(m_device, &m_pbdDescriptorSetLayout);
    m_presolvePipeline.initCompute(m_device, m_pbdPipelineLayout, "assets/spv/presolve.comp.spv");
    m_stretchConstraintPipeline.initCompute(m_device, m_pbdPipelineLayout, "assets/spv/stretch_constraint.comp.spv");
    m_volumeConstraintPipeline.initCompute(m_device, m_pbdPipelineLayout, "assets/spv/volume_constraint.comp.spv");
    m_postsolvePipeline.initCompute(m_device, m_pbdPipelineLayout, "assets/spv/postsolve.comp.spv");

    m_colDescriptorSetLayout.init(m_device,
    {
        {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT }
        },
        {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT }
        }
    });
    m_colDescriptorSet.init(m_device, m_colDescriptorSetLayout, 0, MAX_FRAMES_IN_FLIGHT);
    m_colPipelineLayout.init(m_device, &m_colDescriptorSetLayout);
    m_staticColDetectionPipeline.initCompute(m_device, m_colPipelineLayout, "assets/spv/static_collision_detection.comp.spv");
    m_colConstraintPipeline.initCompute(m_device, m_colPipelineLayout, "assets/spv/collision_constraint.comp.spv");

    m_deformDescriptorSetLayout.init(m_device,
    {
        {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
            { 6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT }
        }
    });
    m_deformPipelineLayout.init(m_device, &m_deformDescriptorSetLayout);
    m_deformPipeline.initCompute(m_device, m_deformPipelineLayout, "assets/spv/deform.comp.spv");
    m_tetDeformPipeline.initCompute(m_device, m_deformPipelineLayout, "assets/spv/tetrahedral_deform.comp.spv");
    m_recalcNormalsPipeline.initCompute(m_device, m_deformPipelineLayout, "assets/spv/recalculate_normals.comp.spv");
    m_normalizeNormalsPipeline.initCompute(m_device, m_deformPipelineLayout, "assets/spv/normalize_normals.comp.spv");

    m_matricesUBO.resize(MAX_FRAMES_IN_FLIGHT);
    m_graphicsUBO.resize(MAX_FRAMES_IN_FLIGHT);
    m_pbdUBO.resize(MAX_FRAMES_IN_FLIGHT);
    m_colUBO.resize(MAX_FRAMES_IN_FLIGHT);

    float dt = 1.0f / (float)m_fixedTimeStep;
    float subdt = dt / m_subSteps;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_matricesUBO[i].init(m_device, {});
        m_graphicsUBO[i].init(m_device, graphics);
        m_pbdUBO[i].init(m_device, { subdt, 0.01f, 0.0f });
        m_colUBO[i].init(m_device, { dt, 0 });
    }

    m_commandPool.init(m_device, VK_PIPELINE_BIND_POINT_GRAPHICS);
    m_commandBufferArray.init(m_device, m_commandPool, MAX_FRAMES_IN_FLIGHT);

    m_computeCommandPool.init(m_device, VK_PIPELINE_BIND_POINT_COMPUTE);
    m_computeCommandBufferArray.init(m_device, m_computeCommandPool, MAX_FRAMES_IN_FLIGHT);

    createSyncObjects();

    m_imGuiRenderer.init(window, m_instance, m_device, m_swapChain, m_commandPool);
    m_shadowRenderer.init(m_device, m_swapChain, m_commandPool, 2048);
    m_shadowSampler.init(m_device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_SAMPLER_MIPMAP_MODE_NEAREST);
    m_resources.init(m_device, m_commandPool);

    createResources();

    m_meshDescriptorSet.writeTexture(0, 0, m_texture, m_sampler);
    m_floorDescriptorSet.writeTexture(0, 0, m_floorTexture, m_sampler);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_graphicsDescriptorSet.writeBuffer(i, 0, m_matricesUBO[i]);
        m_graphicsDescriptorSet.writeBuffer(i, 1, m_graphicsUBO[i]);
        m_graphicsDescriptorSet.writeTexture(i, 2, m_shadowRenderer.getDepthTexture(), m_shadowSampler);
        m_pbdDescriptorSet.writeBuffer(i, 0, m_pbdUBO[i]);
        m_colDescriptorSet.writeBuffer(i, 0, m_colUBO[i]);
        m_colDescriptorSet.writeBuffer(i, 1, m_colPositionsBuffer);
        m_colDescriptorSet.writeBuffer(i, 2, m_colIndicesBuffer);
    }

    m_timer.init(1.0f / m_fixedTimeStep);
}

void Renderer::cleanup()
{
    m_device.waitIdle();
    VkDevice device = m_device.getLogical();

    m_shadowRenderer.cleanup();
    m_imGuiRenderer.cleanup();

    for (auto& softBody : m_softBodies)
        softBody.cleanup();

    m_colIndicesBuffer.cleanup();
    m_colPositionsBuffer.cleanup();

    m_floorTexture.cleanup();
    m_floorMesh.cleanup();

    m_shadowSampler.cleanup();
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
        m_colUBO[i].cleanup();
        m_pbdUBO[i].cleanup();
        m_graphicsUBO[i].cleanup();
        m_matricesUBO[i].cleanup();
    }

    m_normalizeNormalsPipeline.cleanup();
    m_recalcNormalsPipeline.cleanup();
    m_tetDeformPipeline.cleanup();
    m_deformPipeline.cleanup();
    m_deformPipelineLayout.cleanup();
    m_deformDescriptorSetLayout.cleanup();

    m_colConstraintPipeline.cleanup();
    m_staticColDetectionPipeline.cleanup();
    m_colPipelineLayout.cleanup();
    m_colDescriptorSet.cleanup();
    m_colDescriptorSetLayout.cleanup();

    m_postsolvePipeline.cleanup();
    m_volumeConstraintPipeline.cleanup();
    m_stretchConstraintPipeline.cleanup();
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
    if (!m_removeBodies.empty())
    {
        vkQueueWaitIdle(m_device.getComputeQueue());
        vkQueueWaitIdle(m_device.getGraphicsQueue());

        for (auto& softBody : m_removeBodies)
            softBody->cleanup();
        m_removeBodies.clear();
        m_timer.reset();
    }
    if (m_loadSoftBodies == 1) // Normal load
    {
        if (m_modelCount == 1)
        {
            m_softBodies[0] = createSoftBody(m_modelName, m_offset, m_modelResolution);
        }
        else
        {
            float angle = 1.0f;
            float dist = 2.75f;
            for (int i = 0; i < m_modelCount; i++)
            {
                glm::vec3 dir = glm::vec3(sin(angle * i), 0.0f, cos(angle * i));
                glm::vec3 startOffset = dir * (float)(dist * log(i + 2));
                startOffset.y = ((rand() % 1001) * 0.001f) * (m_offset.y - 1) + 1;
                m_softBodies[i] = createSoftBody(m_modelName, startOffset, m_modelResolution);
            }
        }

        m_loadSoftBodies = 0;
        m_timer.reset();
    }
    else if (m_loadSoftBodies == 2) // Error measuring
    {
        m_softBodies[0] = createSoftBody(m_modelName, m_offset, 100);
        m_softBodies[1] = createSoftBody(m_modelName, m_offset, m_modelResolution);

        m_loadSoftBodies = 0;
        m_timer.reset();
    }
    else
        m_timer.update();

    VkDevice device = m_device.getLogical();
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    vkWaitForFences(device, 1, &m_computeInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    vkResetFences(device, 1, &m_computeInFlightFences[currentFrame]);

    m_computeCommandBufferArray.begin(currentFrame);
    if (m_timer.passedFixedDT())
    {
        for (auto& softBody : m_softBodies)
        {
            if (!softBody.active)
                break;

            /*softBody.colSizeBuffer.map();
            uint32_t size = *(uint32_t*)softBody.colSizeBuffer.getMapped();
            softBody.colSizeBuffer.unmap();

            std::cout << "Col size: " << size << "\n";*/
            detectCollisions(m_computeCommandBufferArray[currentFrame], softBody);
        }



        for (auto& softBody : m_softBodies)
        {
            if (!softBody.active)
                break;

            for (int i = 0; i < m_subSteps; i++)
                computePhysics(m_computeCommandBufferArray[currentFrame], softBody);

            deformMesh(m_computeCommandBufferArray[currentFrame], softBody);
        }

        // Measurements
        if (m_measureFrameCounter < MAX_FRAME_MEASUREMENT_COUNT)
        {
            if (m_measureFPS)
            {
                m_avgFPS[m_measureFrameCounter++] = 1.0f / m_timer.getAverageDT();
                if (m_measureFrameCounter == (uint32_t)m_avgFPS.size())
                {
                    std::string path(
                        "../measurements/fps/" +
                        std::string(m_modelName) + "_" +
                        std::to_string(m_modelResolution) + "_" +
                        std::to_string(m_modelCount) + "_" +
                        std::to_string(m_frameCount) + ".txt"
                    );
                    std::ofstream out(path);

                    for (auto& fps : m_avgFPS)
                        out << fps << "\n";

                    out.close();
                    LOG_WRITE("Successfully performed fps measurements");
                    m_measureFrameCounter = MAX_FRAME_MEASUREMENT_COUNT;
                }
            }
            else
            {
                if (m_warmupCounter > 5)
                {
                    std::array<std::vector<avec3>, 2> pos;
                    uint32_t vertexCount = m_softBodies[0].mesh.getVertexCount();
                    for (int i = 0; i < 2; i++)
                    {
                        pos[i].resize(vertexCount);
                        Buffer& buffer = m_softBodies[i].mesh.getVertexBuffer(0);

                        buffer.map();
                        memcpy(pos[i].data(), buffer.getMapped(), buffer.getSize());
                        buffer.unmap();
                    }
                    for (uint32_t i = 0; i < vertexCount; i++)
                    {
                        m_avgError[m_measureFrameCounter] += glm::length(pos[1][i].vec - pos[0][i].vec);
                        m_avgCenterPos[0][m_measureFrameCounter] += pos[0][i].vec;
                        m_avgCenterPos[1][m_measureFrameCounter] += pos[1][i].vec;
                    }
                    m_avgError[m_measureFrameCounter] /= vertexCount;
                    m_avgCenterPos[0][m_measureFrameCounter] /= vertexCount;
                    m_avgCenterPos[1][m_measureFrameCounter] /= vertexCount;
                    m_avgCenterError[m_measureFrameCounter] = glm::length(m_avgCenterPos[1][m_measureFrameCounter] - m_avgCenterPos[0][m_measureFrameCounter]);
                    m_measureFrameCounter++;

                    if (m_measureFrameCounter == (uint32_t)m_avgError.size())
                    {
                        std::string path0(
                            "../measurements/position/" +
                            std::string(m_modelName) + "_100_" +
                            std::to_string(m_frameCount) + ".txt"
                        );
                        std::string path1(
                            "../measurements/position/" +
                            std::string(m_modelName) + "_" +
                            std::to_string(m_modelResolution) + "_" +
                            std::to_string(m_frameCount) + ".txt"
                        );

                        // Center positions
                        std::ofstream out(path0);
                        for (auto& pos : m_avgCenterPos[0])
                            out << pos.x << " " << pos.y << " " << pos.z << "\n";
                        out.close();

                        out.open(path1);
                        for (auto& pos : m_avgCenterPos[1])
                            out << pos.x << " " << pos.y << " " << pos.z << "\n";
                        out.close();

                        std::string errPath(
                            std::string(m_modelName) + "_" +
                            std::to_string(m_modelResolution) + "_" +
                            std::to_string(m_frameCount) + ".txt"
                        );

                        // Error and center error
                        out.open("../measurements/error/" + errPath);
                        for (auto& err : m_avgError)
                            out << err << "\n";
                        out.close();

                        out.open("../measurements/error_center/" + errPath);
                        for (auto& err : m_avgCenterError)
                            out << err << "\n";
                        out.close();

                        LOG_WRITE("Successfully performed error measurements");
                        m_measureFrameCounter = MAX_FRAME_MEASUREMENT_COUNT;
                    }
                }
                else
                    m_warmupCounter++;
            }
        }
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
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(m_device.getPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_swapChain.needsRecreation())
        recreateSwapChain();
    else if (result != VK_SUCCESS)
        LOG_ERROR("Failed to present swap chain image!");

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
