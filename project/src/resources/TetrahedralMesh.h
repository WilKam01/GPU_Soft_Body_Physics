#pragma once

#include "graphics/Buffer.h"
#include "graphics/CommandPool.h"

struct Particle
{
	alignas(16) glm::vec3 position;
	alignas(16) glm::vec3 velocity;
	alignas(4) float invMass;
};

struct Edge
{
	alignas(16) glm::uvec2 indices;
	alignas(4) float restLen;
};

struct Tetrahedral
{
	alignas(16) glm::uvec4 indices;
	alignas(4) float restVolume;
};

struct TetrahedralMeshData
{
	std::vector<Particle> particles;
	std::vector<Tetrahedral> tets;
	std::vector<Edge> edges;
};

class TetrahedralMesh
{
private:
	Device* p_device;
	CommandPool* p_commandPool;
	TetrahedralMeshData m_meshData;

	Buffer m_particleBuffer;
	Buffer m_tetBuffer;
	Buffer m_edgeBuffer;
	Buffer m_pbdPosBuffer;

	uint32_t m_particleCount;
	uint32_t m_tetCount;
	uint32_t m_edgeCount;

	template<typename T>
	void initBuffer(Buffer& buffer, const T* data, uint32_t count);
public:
	void init(Device& device, CommandPool& commandPool, TetrahedralMeshData& meshData);
	void cleanup();
	void reset();

	inline Buffer& getParticleBuffer() { return m_particleBuffer; }
	inline Buffer& getTetBuffer() { return m_tetBuffer; }
	inline Buffer& getEdgeBuffer() { return m_edgeBuffer; }
	inline Buffer& getPbdPosBuffer() { return m_pbdPosBuffer; }

	inline uint32_t getParticleCount() { return m_particleCount; }
	inline uint32_t getTetCount() { return m_tetCount; }
	inline uint32_t getEdgeCount() { return m_edgeCount; }
};

template<typename T>
inline void TetrahedralMesh::initBuffer(Buffer& buffer, const T* data, uint32_t count)
{
	VkDeviceSize bufferSize = sizeof(T) * count;
	Buffer stagingBuffer;
	stagingBuffer.init(*p_device,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		bufferSize,
		(void*)data
	);

	buffer.init(*p_device,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		bufferSize
	);

	p_commandPool->copyBuffer(stagingBuffer, buffer, bufferSize);
	stagingBuffer.cleanup();
}
