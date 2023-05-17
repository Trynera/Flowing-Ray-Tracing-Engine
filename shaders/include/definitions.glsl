#define INFINITY 1. / 0.
#define PI 3.1415926535897932385
#define INV_SQRT_OF_2PI 0.39894228040143267793994605993439
#define INV_PI 0.31830988618379067153776752674503
#define OBJECTS_LIMIT 1024

#define LIGHT_MATERIAL 0
#define ROUGH_MATERIAL 1
#define METAL_MATERIAL 2
#define GLASS_MATERIAL 3

struct Ray {
    vec3 Origin;
    vec3 Direction;
};

struct Material {
    float Roughness;
    float EmissionStrength;
    vec3 Albedo;
    vec3 EmissionAlbedo;
    float IOR; // Index of Refraction
};

struct Triangle {
    vec3 v0;
    vec3 v1;
    vec3 v2;
};

struct AABB {
    vec3 Min;
    vec3 Max;
};

struct Mesh {
    uint TriStartingIndex;
    uint TriCount;
    uint MaterialIndex;
    AABB Box;
};

struct Sphere {
    vec3 Position;
    float Radius;
    uint MaterialIndex;
};

struct HitPayload {
    vec3 HitPoint;
    vec3 WorldNormal;
    float HitDistance;
    bool FrontFace;

    uint MaterialIndex;
};

struct HitList {
    Sphere spheres[OBJECTS_LIMIT];
    Mesh meshes[OBJECTS_LIMIT];
    Triangle triangles[OBJECTS_LIMIT];
    Material materials[OBJECTS_LIMIT];
};

vec3 rayAt(const float t, Ray ray) {
    return ray.Origin + t * ray.Direction;
}

void SetFaceNormal(const Ray ray, const vec3 outwardNormal, inout HitPayload rec) {
    rec.FrontFace = dot(ray.Direction, outwardNormal) < 0;
    rec.WorldNormal = rec.FrontFace ? outwardNormal : -outwardNormal;
}

float reflectance(float cosine, float ref_idx) {
    float r0 = (1-ref_idx) / (1+ref_idx);
    r0 = r0*r0;
    return r0 + (1-r0)*pow((1-cosine), 5);
}