#include "pch.h"
#include "TetrahedralMesh.h"

void TetrahedralMesh::init(Device& device, CommandPool& commandPool, TetrahedralMeshData& meshData)
{
	p_device = &device;
	p_commandPool = &commandPool;
	m_meshData = meshData;
	m_particleCount = (uint32_t)meshData.particles.size();
	m_tetCount = (uint32_t)meshData.tets.size();
	m_edgeCount = (uint32_t)meshData.edges.size();

	initBuffer<Particle>(m_particleBuffer, meshData.particles.data(), m_particleCount);
	initBuffer<Tetrahedral>(m_tetBuffer, meshData.tets.data(), m_tetCount);
	initBuffer<Edge>(m_edgeBuffer, meshData.edges.data(), m_edgeCount);
	initBuffer<Particle>(m_pbdPosBuffer, meshData.particles.data(), m_particleCount);
}

void TetrahedralMesh::cleanup()
{
	m_pbdPosBuffer.cleanup();
	m_edgeBuffer.cleanup();
	m_tetBuffer.cleanup();
	m_particleBuffer.cleanup();
}

void TetrahedralMesh::reset()
{
	m_particleBuffer.map();
	m_particleBuffer.writeTo(m_meshData.particles.data(), m_particleCount * sizeof(Particle));
	m_particleBuffer.unmap();
	
	m_pbdPosBuffer.map();
	m_pbdPosBuffer.writeTo(m_meshData.particles.data(), m_particleCount * sizeof(Particle));
	m_pbdPosBuffer.unmap();
}
