#include "pch.h"
#include "Importer.h"

#define FAST_OBJ_IMPLEMENTATION
#include <fast_obj.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

Device* Importer::s_device = nullptr;
CommandPool* Importer::s_commandPool = nullptr;

void Importer::init(Device& device, CommandPool& commandPool)
{
    s_device = &device;
    s_commandPool = &commandPool;
}

Texture Importer::loadTexture(const std::string& path)
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

void Importer::exportJPG(Texture& texture, const std::string& path)
{
    const char* data;
    glm::uvec2 dimensions = texture.getDimensions();

    vkMapMemory(s_device->getLogical(), texture.getMemory(), 0, VK_WHOLE_SIZE, 0, (void**)&data);
    stbi_write_jpg(path.c_str(), dimensions.x, dimensions.y, 4, data, dimensions.x * 4);
    vkUnmapMemory(s_device->getLogical(), texture.getMemory());
}

Mesh Importer::loadMeshOBJ(const std::string& path, glm::vec3 offset)
{
    Mesh mesh;
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
    std::vector<Vertex> vertices{};
    std::vector<uint32_t> indices(obj->index_count);

    for (unsigned int i = 0; i < obj->index_count; i++)
    {
        std::string key = std::to_string(obj->indices[i].p) + std::to_string(obj->indices[i].n) + std::to_string(obj->indices[i].t);

        Vertex vertex;
        vertex.position = glm::vec3(
            obj->positions[obj->indices[i].p * 3],
            obj->positions[obj->indices[i].p * 3 + 1],
            obj->positions[obj->indices[i].p * 3 + 2]
        );
        vertex.normal = glm::vec3(
            obj->normals[obj->indices[i].n * 3],
            obj->normals[obj->indices[i].n * 3 + 1],
            obj->normals[obj->indices[i].n * 3 + 2]
        );
        vertex.uv = glm::vec2(
            obj->texcoords[obj->indices[i].t * 2],
            obj->texcoords[obj->indices[i].t * 2 + 1]
        );

        vertex.position += offset;

        if (!uniqueVertices.count(key))
        {
            uniqueVertices[key] = static_cast<uint32_t>(vertices.size());
            vertices.push_back(vertex);
        }

        indices[i] = uniqueVertices[key];
    }

    mesh.init(*s_device, *s_commandPool, vertices, indices);

    fast_obj_destroy(obj);
    return mesh;
}
