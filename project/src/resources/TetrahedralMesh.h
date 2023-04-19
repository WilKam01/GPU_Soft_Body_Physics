#pragma once

#include "graphics/Buffer.h"
#include "graphics/CommandPool.h"

struct Particle
{
	glm::vec4 position;
	glm::vec4 velocity;
};

struct Edge
{
	glm::uvec2 indices;
	float restLen;
	float pad;
};

struct TetrahedralMeshData
{
	std::vector<Particle> particles;
	std::vector<glm::uvec4> tetIds;
	std::vector<Edge> edges;
	//std::vector<uint32_t> surfaceIndices;
};

class TetrahedralMesh
{
private:
	Device* p_device;
	const TetrahedralMeshData* p_meshData;

	Buffer m_particleBuffer;
	Buffer m_tetIdBuffer;
	Buffer m_edgeBuffer;
	Buffer m_predictPosBuffer;
	//Buffer m_indexBuffer;

	uint32_t m_particleCount;
	uint32_t m_tetCount;
	//uint32_t m_indexCount;

public:
	void bindGraphics();

	void init(Device& device, CommandPool& commandPool, const TetrahedralMeshData& meshData);
	void cleanup();

	inline Buffer& getParticleBuffer() { return m_particleBuffer; }
	inline Buffer& getTetIdBuffer() { return m_tetIdBuffer; }
	inline Buffer& getPredictPosBuffer() { return m_predictPosBuffer; }

	inline uint32_t getParticleCount() { return m_particleCount; }
	inline uint32_t getTetCount() { return m_tetCount; }
};
