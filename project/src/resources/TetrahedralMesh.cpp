#include "pch.h"
#include "TetrahedralMesh.h"

void TetrahedralMesh::init(Device& device, CommandPool& commandPool, TetrahedralMeshData& meshData)
{
	p_device = &device;
	p_commandPool = &commandPool;
	m_meshData = meshData;
	m_particleCount = (uint32_t)meshData.particles.size();
	m_tetCount = (uint32_t)meshData.tetIds.size();
	m_edgeCount = (uint32_t)meshData.edges.size();

	std::vector<glm::vec4> predictPositions(m_particleCount);
	for (uint32_t i = 0; i < m_particleCount; i++)
		predictPositions[i] = meshData.particles[i].position;

	initBuffer<Particle>(m_particleBuffer, meshData.particles.data(), m_particleCount);
	initBuffer<glm::uvec4>(m_tetIdBuffer, meshData.tetIds.data(), m_tetCount);
	initBuffer<Edge>(m_edgeBuffer, meshData.edges.data(), m_edgeCount);
	initBuffer<glm::vec4>(m_predictPosBuffer, predictPositions.data(), m_particleCount);
	
	m_deltaPosBuffer.init(*p_device,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		sizeof(glm::vec4) * m_particleCount
	);
}

void TetrahedralMesh::cleanup()
{
	m_deltaPosBuffer.cleanup();
	m_predictPosBuffer.cleanup();
	m_edgeBuffer.cleanup();
	m_tetIdBuffer.cleanup();
	m_particleBuffer.cleanup();
}

void TetrahedralMesh::reset()
{
	std::vector<glm::vec4> predictPositions(m_particleCount);
	for (uint32_t i = 0; i < m_particleCount; i++)
		predictPositions[i] = m_meshData.particles[i].position;

	m_particleBuffer.map();
	m_particleBuffer.writeTo(m_meshData.particles.data(), m_particleCount * sizeof(Particle));
	m_particleBuffer.unmap();
	
	m_predictPosBuffer.map();
	m_predictPosBuffer.writeTo(predictPositions.data(), m_particleCount * sizeof(glm::vec4));
	m_predictPosBuffer.unmap();
}
