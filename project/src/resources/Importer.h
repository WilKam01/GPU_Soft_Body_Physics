#pragma once

#include "Texture.h"
#include "Mesh.h"

class Importer
{
private:
	static Device* s_device;
	static CommandPool* s_commandPool;
public:
	static void init(Device& device, CommandPool& commandPool);

	static Texture loadTexture(const std::string& path);
	static void exportJPG(Texture& texture, const std::string& path);

	// Note: The face format in the obj file must be triangular
	static Mesh loadMeshOBJ(const std::string& path, glm::vec3 offset = glm::vec3(0.0f));
};

