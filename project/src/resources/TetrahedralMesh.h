#pragma once

#include "graphics/Buffer.h"
#include "graphics/CommandPool.h"

struct TetrahedralMeshData
{
	std::vector<glm::vec4> positions;
	std::vector<glm::uvec4> tetIds;
	//std::vector<glm::ivec2> edges;
	std::vector<uint32_t> surfaceIndices;
};

class TetrahedralMesh
{
private:
	Device* p_device;
	const TetrahedralMeshData* p_meshData;

	Buffer m_positionBuffer;
	Buffer m_tetIdBuffer;
	Buffer m_indexBuffer;

	uint32_t m_vertexCount;
	uint32_t m_tetCount;
	uint32_t m_indexCount;

public:
	void bindGraphics();

	void init(Device& device, CommandPool& commandPool, const TetrahedralMeshData& meshData);
	void cleanup();

	inline Buffer& getPositionBuffer() { return m_positionBuffer; }
	inline Buffer& getTetIdBuffer() { return m_tetIdBuffer; }
	inline uint32_t getTetCount() { return m_tetCount; }
};
