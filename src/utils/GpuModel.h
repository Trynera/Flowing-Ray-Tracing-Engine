#pragma once

#include <glm/glm.hpp>

#define OBJECTS_LIMIT 1024

// This is where every required Structure is defined.
// Material - Defines the Objects Properties.
// Triangle - A single Triangle.
// AABB - Defines an axis-aligned bounding box, for optimization reasons.
// Mesh - Defines a multiple of Triangles in a single Struct.
// Sphere - It's a Sphere idk what to tell you.
// HitList - Has an Array of all possible Objects.

struct Material {
	alignas(4) float Roughness;
	alignas(4) float EmissionStrength;
	alignas(16) glm::vec3 Albedo;
	alignas(16) glm::vec3 EmissionAlbedo;
	alignas(4) float IOR;
};

struct Triangle {
	alignas(16) glm::vec3 v0;
	alignas(16) glm::vec3 v1;
	alignas(16) glm::vec3 v2;
};

struct AABB {
	alignas(16) glm::vec3 Min;
	alignas(16) glm::vec3 Max;
};

struct GpuMesh {
	alignas(4) uint32_t TriStartingIndex;
	alignas(4) uint32_t TriCount;
	alignas(4) uint32_t MaterialIndex;
	alignas(16) AABB Box;
};

struct Sphere {
	alignas(16) glm::vec3 Position;
	alignas(4) float Radius;
	alignas(4) uint32_t MaterialIndex;
};

struct HitList {
	Sphere spheres[OBJECTS_LIMIT];
	GpuMesh meshes[OBJECTS_LIMIT];
	Triangle triangles[OBJECTS_LIMIT];
	Material materials[OBJECTS_LIMIT];
};