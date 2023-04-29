#include "pch.h"
#include "Mesh.h"

void Mesh::init(Device& device, CommandPool& commandPool, MeshData& meshData)
{
    p_device = &device;
    m_meshData = meshData;
    m_vertexCount = (uint32_t)meshData.vertices.positions.size();
    m_indexCount = (uint32_t)meshData.indices.size();

    // Vertex buffers
    addVertexBuffer<avec3, avec3>(commandPool, meshData.vertices.positions, true);
    addVertexBuffer<avec3, avec3>(commandPool, meshData.vertices.normals, true);
    addVertexBuffer<glm::vec2, glm::vec2>(commandPool, meshData.vertices.uvs);

    // Index buffer
    Buffer stagingBuffer;
    VkDeviceSize bufferSize = sizeof(uint32_t) * m_indexCount;
    stagingBuffer.init(device,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        bufferSize,
        (void*)meshData.indices.data()
    );

    m_indexBuffer.init(device,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        bufferSize
    );

    commandPool.copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);
    stagingBuffer.cleanup();

    m_bufferCount = (uint32_t)m_vertexBuffers.size();
    m_rawVertexBuffers.resize(m_bufferCount);
    m_offsets.resize(m_bufferCount, 0);
    for (uint32_t i = 0; i < m_bufferCount; i++)
        m_rawVertexBuffers[i] = m_vertexBuffers[i].get();
}

void Mesh::cleanup()
{
    m_indexBuffer.cleanup();
    for(auto& buffer : m_vertexBuffers)
        buffer.cleanup();
}

void Mesh::bind(VkCommandBuffer commandBuffer)
{
    vkCmdBindVertexBuffers(commandBuffer, 0, m_bufferCount, m_rawVertexBuffers.data(), m_offsets.data());
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer.get(), 0, VK_INDEX_TYPE_UINT32);
}
