// ATTENTION !!!
// THIS IS NOT MY CODE, CHECK OUT THIS PROJECT: https://github.com/grigoryoskin/vulkan-compute-ray-tracing

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <vector>
#include <unordered_map>
#include <array>
#include <string>
#include "mesh.h"

bool Vertex::operator==(const Vertex& other) const
{
    return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
}

void Mesh::initPlane()
{
    float quadVertices[] = {
        // positions        // texture Coords
        -1.0f,
        -1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        -1.0f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        1.0f,
        0.0f,
        1.0f,
        1.0f,
        -1.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
    };

    for (int i = 0; i < 4; i++)
    {
        Vertex v{};
        v.pos = glm::vec3(quadVertices[5 * i], quadVertices[5 * i + 1], quadVertices[5 * i + 2]);
        v.normal = glm::vec3(0.0f, 0.0f, 0.0f);
        v.texCoord = glm::vec2(quadVertices[5 * i + 3], quadVertices[5 * i + 4]);
        vertices.push_back(v);
    }

    indices = { 0, 3, 2, 2, 1, 0 };
}

Mesh::Mesh(std::vector<Triangle>* tris, std::string model_path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str()))
    {
        throw std::runtime_error(warn + err);
    }

    uint32_t triCount = uint32_t(attrib.vertices.size() * 3);
    size_t triStartIndex = sizeof(tris);

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2] };

            // vertex.texCoord = {
            //     attrib.texcoords[2 * index.texcoord_index + 0],
            //     1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

            if (uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }
}