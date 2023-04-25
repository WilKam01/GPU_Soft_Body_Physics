#include "pch.h"
#include "ResourceManager.h"

#define FAST_OBJ_IMPLEMENTATION
#include <fast_obj.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

Device* ResourceManager::s_device = nullptr;
CommandPool* ResourceManager::s_commandPool = nullptr;

void ResourceManager::init(Device& device, CommandPool& commandPool)
{
    s_device = &device;
    s_commandPool = &commandPool;
}

Texture ResourceManager::loadTexture(const std::string& path)
{
    Texture texture;

    int width = 0, height = 0, channels = 0;
    stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data)
    {
        LOG_WARNING("Failed to load texture with image path: " + path);
        return texture;
    }

    texture.init(*s_device, *s_commandPool, data, glm::uvec2(width, height));
    stbi_image_free(data);

	return texture;
}

void ResourceManager::exportJPG(Texture& texture, const std::string& path)
{
    const char* data;
    glm::uvec2 dimensions = texture.getDimensions();

    vkMapMemory(s_device->getLogical(), texture.getMemory(), 0, VK_WHOLE_SIZE, 0, (void**)&data);
    stbi_write_jpg(path.c_str(), dimensions.x, dimensions.y, 4, data, dimensions.x * 4);
    vkUnmapMemory(s_device->getLogical(), texture.getMemory());
}

MeshData ResourceManager::loadMeshOBJ(const std::string& path, glm::vec3 offset)
{
    MeshData mesh;
    fastObjMesh* obj = fast_obj_read(path.c_str());

    if (!obj)
    {
        LOG_WARNING("Failed to load obj mesh with path: " + path);
        return mesh;
    }
    else if (obj->face_vertices[0] != 3)
    {
        LOG_WARNING("Failed to construct mesh from obj mesh: " + path + " The faces are not triangular!");
        return mesh;
    }

    std::unordered_map<std::string, uint32_t> uniqueVertices{};
    mesh.indices.resize(obj->index_count);
    mesh.origIndices.resize(obj->index_count);
    mesh.origPositions.resize(obj->position_count - 1);

    for (unsigned int i = 0; i < obj->index_count; i++)
    {
        std::string key = std::to_string(obj->indices[i].p) + std::to_string(obj->indices[i].n) + std::to_string(obj->indices[i].t);
        if (!uniqueVertices.count(key))
        {
            uniqueVertices[key] = static_cast<uint32_t>(mesh.vertices.positions.size());
            mesh.vertices.positions.push_back(glm::vec4(
                obj->positions[obj->indices[i].p * 3],
                obj->positions[obj->indices[i].p * 3 + 1],
                obj->positions[obj->indices[i].p * 3 + 2],
                0.0f
            ) + glm::vec4(offset, 0.0f));
            mesh.vertices.normals.push_back(glm::vec3(
                obj->normals[obj->indices[i].n * 3],
                obj->normals[obj->indices[i].n * 3 + 1],
                obj->normals[obj->indices[i].n * 3 + 2]
            ));
            mesh.vertices.uvs.push_back(glm::vec2(
                obj->texcoords[obj->indices[i].t * 2],
                obj->texcoords[obj->indices[i].t * 2 + 1]
            ));
        }

        mesh.indices[i] = uniqueVertices[key];
        mesh.origIndices[i] = obj->indices[i].p;
    }
    for (unsigned int i = 1; i < obj->position_count; i++)
    {
        mesh.origPositions[i - 1] = glm::vec3(
            obj->positions[i * 3],
            obj->positions[i * 3 + 1],
            obj->positions[i * 3 + 2]
        ) + offset;
    }

    fast_obj_destroy(obj);
    return mesh;
}

TetrahedralMeshData ResourceManager::loadTetrahedralMeshOBJ(const std::string& path, glm::vec3 offset)
{
    TetrahedralMeshData mesh;
    fastObjMesh* obj = fast_obj_read(path.c_str());

    if (!obj)
    {
        LOG_WARNING("Failed to load obj tetrahedral mesh with path: " + path);
        return mesh;
    }
    else if (obj->face_vertices[0] != 4)
    {
        LOG_WARNING("Failed to construct tetrahedral mesh from obj mesh: " + path + " Each \"face\" in the obj file should be used to define a tetrahedral.");
        return mesh;
    }

    mesh.particles.resize(obj->position_count - 1);
    mesh.tetIds.resize(obj->index_count / 4);
    mesh.edges.resize(mesh.tetIds.size() * 6);

    for (unsigned int i = 1; i < obj->position_count; i++)
    {
        mesh.particles[i - 1].position = glm::vec4(
            obj->positions[i * 3],
            obj->positions[i * 3 + 1],
            obj->positions[i * 3 + 2],
            0.0f
        ) + glm::vec4(offset, 0.0f);
    }
    for (int i = 0, len = (int)mesh.tetIds.size(); i < len; i++)
    {
        glm::uvec4 ids(
            obj->indices[4 * i].p - 1,
            obj->indices[4 * i + 1].p - 1,
            obj->indices[4 * i + 2].p - 1,
            obj->indices[4 * i + 3].p - 1
        );
        mesh.tetIds[i] = ids;

        mesh.edges[6 * i + 0] = { glm::uvec2(ids[0], ids[1]), glm::length(mesh.particles[ids[0]].position - mesh.particles[ids[1]].position)};
        mesh.edges[6 * i + 1] = { glm::uvec2(ids[0], ids[2]), glm::length(mesh.particles[ids[0]].position - mesh.particles[ids[2]].position)};
        mesh.edges[6 * i + 2] = { glm::uvec2(ids[0], ids[3]), glm::length(mesh.particles[ids[0]].position - mesh.particles[ids[3]].position)};
        mesh.edges[6 * i + 3] = { glm::uvec2(ids[1], ids[2]), glm::length(mesh.particles[ids[1]].position - mesh.particles[ids[2]].position)};
        mesh.edges[6 * i + 4] = { glm::uvec2(ids[1], ids[3]), glm::length(mesh.particles[ids[1]].position - mesh.particles[ids[3]].position)};
        mesh.edges[6 * i + 5] = { glm::uvec2(ids[2], ids[3]), glm::length(mesh.particles[ids[2]].position - mesh.particles[ids[3]].position)};
    }

    fast_obj_destroy(obj);
    return mesh;
}