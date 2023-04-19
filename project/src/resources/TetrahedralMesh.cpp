#include "pch.h"
#include "TetrahedralMesh.h"

void TetrahedralMesh::init(Device& device, CommandPool& commandPool, const TetrahedralMeshData& meshData)
{
	p_device = &device;
	p_meshData = &meshData;
	m_particleCount = (uint32_t)meshData.particles.size();
	m_tetCount = (uint32_t)meshData.tetIds.size();
	//m_indexCount = (uint32_t)meshData.surfaceIndices.size();

	VkDeviceSize bufferSize = sizeof(Particle) * m_particleCount;
	Buffer stagingBuffer;
	stagingBuffer.init(*p_device,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		bufferSize,
		(void*)meshData.particles.data()
	);

	m_particleBuffer.init(*p_device,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		bufferSize
	);

	commandPool.copyBuffer(stagingBuffer, m_particleBuffer, bufferSize);
	stagingBuffer.cleanup();

	bufferSize = sizeof(glm::uvec4) * m_tetCount;
	stagingBuffer.init(device,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		bufferSize,
		(void*)meshData.tetIds.data()
	);

	m_tetIdBuffer.init(device,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		bufferSize
	);

	commandPool.copyBuffer(stagingBuffer, m_tetIdBuffer, bufferSize);
	stagingBuffer.cleanup();

	std::vector<glm::vec4> predictPositions(m_particleCount);
	for (uint32_t i = 0; i < m_particleCount; i++)
		predictPositions[i] = meshData.particles[i].position;
		
	bufferSize = sizeof(glm::vec4) * m_particleCount;
	stagingBuffer.init(device,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		bufferSize,
		(void*)predictPositions.data()
	);

	m_predictPosBuffer.init(device,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		bufferSize
	);

	commandPool.copyBuffer(stagingBuffer, m_predictPosBuffer, bufferSize);
	stagingBuffer.cleanup();
}

void TetrahedralMesh::cleanup()
{
	m_predictPosBuffer.cleanup();
	m_tetIdBuffer.cleanup();
	m_particleBuffer.cleanup();
}
