#pragma once

#include "Texture.h"
#include "Mesh.h"
#include "TetrahedralMesh.h"

struct DeformationInfo
{
	alignas(16) glm::vec3 weights = glm::vec3(0.0f);
	alignas(4) uint32_t tetId = 0;
};

struct SoftBodyData
{
	MeshData* mesh;
	TetrahedralMeshData tetMesh;
	std::vector<DeformationInfo> deformationInfo;
};

class ResourceManager
{
private:
	Device* s_device;
	CommandPool* s_commandPool;
	std::unordered_map<std::string, MeshData> m_meshModels;
	std::unordered_map<std::string, SoftBodyData> m_softBodyModels;
public:
	void init(Device& device, CommandPool& commandPool);

	Texture loadTexture(const std::string& path);
	void exportJPG(Texture& texture, const std::string& path);

	// Note: The face format in the obj file must be triangular
	MeshData loadMeshOBJ(const std::string& path);

	// Note: The face format in the obj file must be quadratic
	TetrahedralMeshData loadTetrahedralMeshOBJ(const std::string& path);

	SoftBodyData* getSoftBody(std::string name, int resolution);
};

