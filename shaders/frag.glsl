#version 450 core

layout(location = 0) in vec2 in_position;

layout(location = 0) out vec4 out_color;

#include "include/definitions.glsl"
#include "include/random.glsl"

#define SAMPLES_PER_PIXEL 8

HitList world;
int objectCount;

bool hitSphere(const Ray ray, float t_min, float t_max, inout HitPayload rec, Sphere sphere) {
    vec3 oc = ray.Origin - sphere.Position;
    float a = length(ray.Direction) * length(ray.Direction);
    float half_b = dot(oc, ray.Direction);
    float c = length(oc) * length(oc) - sphere.Radius*sphere.Radius;

    float discriminant = half_b*half_b - a*c;
    if (discriminant < 0)
        return false;
    float sqrtd = sqrt(discriminant);

    float root = (-half_b - sqrtd) / a;
    if (root < t_min || t_max < root) {
        root = (-half_b + sqrtd) / a;
        if (root < t_min || t_max < root)
            return false;
    }

    rec.HitDistance = root;
    rec.HitPoint = rayAt(root, ray);
    vec3 OutwardNormal = (rec.HitPoint - sphere.Position) / sphere.Radius;
    SetFaceNormal(ray, OutwardNormal, rec);
    
    return true;
}

vec3 triIntersect(const Ray ray, Triangle tri, inout vec3 n) {
    vec3 a = tri.v0 - tri.v1;
    vec3 b = tri.v2 - tri.v0;
    vec3 p = tri.v0 - ray.Origin;
    n = cross( b, a );

    vec3 q = cross( p, ray.Direction );

    float idet = 1.0/dot( ray.Direction, n );

    float u = dot( q, b )*idet;
    float v = dot( q, a )*idet;
    float t = dot( n, p )*idet;

    return vec3( t, u, v );
}

bool hitTriangle(const Ray ray, float t_min, float t_max, Triangle tri, inout HitPayload rec) {
    vec3 n = vec3(0,0,0);
    vec3 hit = triIntersect(ray, tri, n);
    
    if (hit.y<0.0 || hit.y>1.0 || hit.z<0.0 || (hit.y+hit.z)>1.0)
        return false;
    
    rec.HitPoint = ray.Origin + hit.x * ray.Direction;
    rec.WorldNormal =  normalize(n);
    rec.FrontFace = dot(ray.Direction,rec.WorldNormal) > 0 ? false : true;
    rec.WorldNormal *= 1 - 2 * -(int(rec.FrontFace) - 1);
    rec.HitPoint += rec.WorldNormal*0.0001;
    rec.HitDistance = hit.x;
    rec.MaterialIndex = tri.MaterialIndex;
    return hit.x > t_min && hit.x < t_max;
}

bool Hit(const Ray ray, float t_min, float t_max, inout HitPayload rec) {
    HitPayload tempRec;
    bool hitAnything = false;
    float closestObject = t_max;

    for (int i = 0; i < 1; i++) {
        if (hitSphere(ray, t_min, closestObject, tempRec, world.spheres[i])) {
            hitAnything = true;
            closestObject = tempRec.HitDistance;
            rec = tempRec;
        }
    }

    closestObject = t_max;

    for (int i = 0; i < 3; i++) {
        if (hitTriangle(ray, t_min, t_max, world.triangles[i], tempRec)) {
            hitAnything = true;
            closestObject = tempRec.HitDistance;
            rec = tempRec;
        }
    }

    return hitAnything;
}

#define MAX_BOUNCES 5
vec3 rayColor(const Ray ray) {
    HitPayload rec;
    if (Hit(ray, 0.001, INFINITY, rec))
        return 0.5 * (rec.WorldNormal + vec3(1));

    float t = 0.5*(ray.Direction.y + 1.0);
    return (1.0-t)*vec3(1.0) + t*vec3(0.5, 0.7, 1.0);
}

void main() {
    world.spheres[0].Position = vec3(0, -100.5, -1);
    world.spheres[0].Radius = 100;

    world.triangles[0].v0 = vec3(-1, -0.5, -1);
    world.triangles[0].v1 = vec3(1, -0.5, -1);
    world.triangles[0].v2 = vec3(0, 0.5, -1);
    
    Ray ray;
    ray.Origin = vec3(0, 0, 0);

    for (int i = 0; i < SAMPLES_PER_PIXEL; i++) {
        vec2 delta = vec2(i%3-1, i/3-1);
        ray.Direction = vec3(((in_position.x * 1280) + delta.x/3) / 720, ((-in_position.y * 720) + delta.y/3) / 720, -1);
        out_color += vec4(rayColor(ray), 1.0);
    }
    
    out_color /= SAMPLES_PER_PIXEL;
}