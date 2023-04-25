#pragma once

#include "graphics/Buffer.h"
#include "graphics/CommandPool.h"

enum VertexStreamInput 
{
	VERTEX_STREAM_INPUT_NONE = 0,
	VERTEX_STREAM_INPUT_POSITION = 1,
	VERTEX_STREAM_INPUT_NORMAL = 2,
	VERTEX_STREAM_INPUT_UV = 4,
	VERTEX_STREAM_INPUT_ALL = ~0u,
};

struct VertexStream
{
	std::vector<glm::vec4> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uvs;
};

struct MeshData
{
	VertexStream vertices;
	std::vector<uint32_t> indices;

	// "Raw" positions and indices, meaning original data from input file
	std::vector<glm::vec3> origPositions;
	std::vector<uint32_t> origIndices;
};

class Mesh
{
private:
	Device* p_device;
	MeshData m_meshData;

	std::vector<Buffer> m_vertexBuffers;
	Buffer m_indexBuffer;
	uint32_t m_vertexCount;
	uint32_t m_indexCount;

	std::vector<VkBuffer> m_rawVertexBuffers;
	std::vector<VkDeviceSize> m_offsets;
	uint32_t m_bufferCount;

	template <typename T>
	void addVertexBuffer(CommandPool& commandPool, const std::vector<T>& stream, bool isSBO = false);
public:
	void init(Device& device, CommandPool& commandPool, MeshData& meshData);
	void cleanup();

	void bind(VkCommandBuffer commandBuffer);
	
	inline Buffer& getVertexBuffer(size_t i) { return m_vertexBuffers[i]; }
	inline Buffer& getIndexBuffer() { return m_indexBuffer; }
	inline uint32_t getVertexCount() { return m_vertexCount; }
	inline uint32_t getIndexCount() { return m_indexCount; }
};

template<typename T>
inline void Mesh::addVertexBuffer(CommandPool& commandPool, const std::vector<T>& stream, bool isSBO)
{
	if (stream.size() == 0)
		return;

	VkDeviceSize bufferSize = sizeof(stream[0]) * stream.size();
	Buffer stagingBuffer;
	stagingBuffer.init(*p_device,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		bufferSize,
		(void*)stream.data()
	);

	m_vertexBuffers.push_back(Buffer());
	m_vertexBuffers.back().init(*p_device,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | isSBO * VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		bufferSize
	);

	commandPool.copyBuffer(stagingBuffer, m_vertexBuffers.back(), bufferSize);

	stagingBuffer.cleanup();
}
