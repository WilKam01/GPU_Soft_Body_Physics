#pragma once

#include "Buffer.h"
#include "CommandPool.h"

struct Vertex
{
	glm::vec3 position;
	glm::vec2 uv;
};

class Mesh
{
private:
	Device* p_device;

	Buffer m_vertexBuffer;
	Buffer m_indexBuffer;
	uint32_t m_vertexCount;
	uint32_t m_indexCount;
public:
	void init(Device& device, CommandPool& commandPool, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
	void cleanup();

	void bind(VkCommandBuffer commandBuffer);
	
	inline Buffer& getVertexBuffer() { return m_vertexBuffer; }
	inline Buffer& getIndexBuffer() { return m_indexBuffer; }
	inline uint32_t getVertexCount() { return m_vertexCount; }
	inline uint32_t getIndexCount() { return m_indexCount; }
};

