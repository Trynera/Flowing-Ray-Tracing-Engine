#define INFINITY 1. / 0.
#define PI 3.1415926535897932385
#define OBJECTS_LIMIT 128

struct Ray {
    vec3 Origin;
    vec3 Direction;
};

struct Material {
    uint MaterialType;
    vec3 Albedo;
};

struct Triangle {
    vec3 v0;
    vec3 v1;
    vec3 v2;
    uint MaterialIndex;
};

struct LightPoint {
    uint TriangleIndex;
    float Area;
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

struct Object {
    vec3 Position;
    Triangle triangles[OBJECTS_LIMIT];
    uint MaterialIndex;
};

struct HitList {
    Sphere spheres[OBJECTS_LIMIT];
    Triangle triangles[OBJECTS_LIMIT];
};

struct BVHNode {
    vec3 Min;
    vec3 Max;
    int LeftNodeIndex;
    int RightNodeIndex;
    int ObjectIndex;
};

struct ONB {
    vec3 u;
    vec3 v;
    vec3 w;
};

#define LIGHT_MATERIAL 0
#define LAMBERTIAN_MATERIAL 1
#define METAL_MATERIAL 2
#define GLASS_MATERIAL 3

vec3 rayAt(const float t, Ray ray) {
    return ray.Origin + t * ray.Direction;
}

ONB onb(vec3 n) {
    ONB res;
    res.w = normalize(n);
    vec3 a = (abs(res.w.x) > 0.9) ? vec3(0, 1, 0) : vec3(1, 0, 0);
    res.v = normalize(cross(res.w, a));
    res.u = cross(res.w, res.v);
    return res;
}

vec3 onbLocal(vec3 a, ONB o) {
    return a.x * o.u + a.y * o.v + a.z * o.w;
}

void SetFaceNormal(const Ray ray, const vec3 outwardNormal, inout HitPayload rec) {
    rec.FrontFace = dot(ray.Direction, outwardNormal) < 0;
    rec.WorldNormal = rec.FrontFace ? outwardNormal : -outwardNormal;
}