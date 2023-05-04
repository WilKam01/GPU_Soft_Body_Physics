#include "pch.h"
#include "TetrahedralMesh.h"

void TetrahedralMesh::init(Device& device, CommandPool& commandPool, TetrahedralMeshData* meshData, glm::vec3 offset)
{
	p_device = &device;
	p_commandPool = &commandPool;
	p_meshData = meshData;
	m_particleCount = (uint32_t)meshData->particles.size();
	m_tetCount = (uint32_t)meshData->tets.size();
	m_edgeCount = (uint32_t)meshData->edges.size();

	std::vector<Particle> particleData(meshData->particles);
	for (auto& particle : particleData)
		particle.position += offset;

	initBuffer<Particle>(m_particleBuffer, particleData.data(), m_particleCount);
	initBuffer<Tetrahedral>(m_tetBuffer, meshData->tets.data(), m_tetCount);
	initBuffer<Edge>(m_edgeBuffer, meshData->edges.data(), m_edgeCount);
	initBuffer<Particle>(m_pbdPosBuffer, particleData.data(), m_particleCount);
}

void TetrahedralMesh::cleanup()
{
	m_pbdPosBuffer.cleanup();
	m_edgeBuffer.cleanup();
	m_tetBuffer.cleanup();
	m_particleBuffer.cleanup();
}
