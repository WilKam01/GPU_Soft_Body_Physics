#include "pch.h"
#include "Mesh.h"

void Mesh::init(Device& device, CommandPool& commandPool, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    p_device = &device;
    m_vertexCount = (uint32_t)vertices.size();
    m_indexCount = (uint32_t)indices.size();

    // Vertex buffer
    VkDeviceSize bufferSize = sizeof(Vertex) * m_vertexCount;
    Buffer stagingBuffer;
    stagingBuffer.init(device,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        bufferSize,
        (void*)vertices.data()
    );

    m_vertexBuffer.init(device,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        bufferSize
    );

    commandPool.copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

    stagingBuffer.cleanup();

    // Index buffer
    bufferSize = sizeof(uint32_t) * m_indexCount;
    stagingBuffer.init(device,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        bufferSize,
        (void*)indices.data()
    );

    m_indexBuffer.init(device,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        bufferSize
    );

    commandPool.copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

    stagingBuffer.cleanup();
}

void Mesh::cleanup()
{
    m_indexBuffer.cleanup();
    m_vertexBuffer.cleanup();
}

void Mesh::bind(VkCommandBuffer commandBuffer)
{
    VkBuffer vertexBuffers[] = { m_vertexBuffer.get() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer.get(), 0, VK_INDEX_TYPE_UINT32);
}
