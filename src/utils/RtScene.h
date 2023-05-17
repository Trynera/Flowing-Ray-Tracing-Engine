#pragma once

#include <vector>
#include <string>
#include <iostream>

#include "mesh.h"
#include "GpuModel.h"

// ATTENTION !!!
// THIS IS NOT MY CODE, CHECK OUT THIS PROJECT: https://github.com/grigoryoskin/vulkan-compute-ray-tracing

void createTriangles(std::vector<Triangle>* tris, const std::string& path)
{
    Mesh mesh(tris, path);

    std::vector<Triangle> triangles;
    int numTris = mesh.indices.size() / 3;
    AABB boundingBox = {};

    for (uint32_t i = 0; i < numTris; i++)
    {
        int i0 = mesh.indices[3 * i];
        int i1 = mesh.indices[3 * i + 1];
        int i2 = mesh.indices[3 * i + 2];

        Triangle t{ mesh.vertices[i0].pos, mesh.vertices[i1].pos, mesh.vertices[i2].pos };
        tris->push_back(t);
    }
}