#pragma once

#include "GpuModel.h"
#include <vector>
#include <unordered_map>
#include <array>
#include <string>
#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"
#include "vulkan/vulkan.h"

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;

    bool operator==(const Vertex& other) const;
};

namespace std
{
    template <>
    struct hash<Vertex>
    {
        size_t operator()(Vertex const& vert) const
        {
            return ((hash<glm::vec3>()(vert.pos) ^
                (hash<glm::vec3>()(vert.normal) << 1)) >>
                1) ^
                (hash<glm::vec2>()(vert.texCoord) << 1);
        }
    };
}

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    Mesh() = default;

    Mesh(std::vector<Triangle>* tris, std::string model_path);

    void initPlane();
};