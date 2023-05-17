#version 460 core

layout(location = 0) in vec2 in_position;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform UniformBufferObject {
    vec3 camPos;
    mat4 rotationMatrix;
    ivec2 screenRes;
    float time;
} ubo;

#include "include/definitions.glsl"
#include "include/random.glsl"

layout(binding = 1) readonly buffer TriangleBuffer {
    Triangle[] triangles;
} TBuffer;

HitList world;

AABB combineAABB(const AABB boxA, const AABB boxB) {
    vec3 Min = min(boxA.Min, boxB.Min);
    vec3 Max = max(boxA.Max, boxB.Min);
    return AABB(Min, Max);
}

bool hitAABB(const Ray ray, AABB box) {
    vec3 invDir = 1 / ray.Direction;
    vec3 t_min = (box.Min - ray.Origin) * invDir;
    vec3 t_max = (box.Max - ray.Origin) * invDir;
    vec3 t1 = min(t_min, t_max);
    vec3 t2 = max(t_min, t_max);
    float t_near = max(max(t1.x, t1.y), t1.z);
    float t_far = min(min(t2.x, t2.y), t2.z);
    return t_near <= t_far;
}

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
    rec.MaterialIndex = sphere.MaterialIndex;
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

    float idet = 1.0 / (dot(ray.Direction, n));

    float u = dot( q, b )*idet;
    float v = dot( q, a )*idet;
    float t = dot( n, p )*idet;

    return vec3( t, u, v );
}

bool hitTriangle(const Ray ray, float t_min, float t_max, Triangle tri, AABB box, inout HitPayload rec) {
    vec3 n = vec3(0);
    vec3 hit = triIntersect(ray, tri, n);
    
    if (hit.y<0.0 || hit.y>1.0 || hit.z<0.0 || (hit.y+hit.z)>1.0)
        return false;
    
    rec.HitPoint = ray.Origin + hit.x * ray.Direction;
    rec.WorldNormal = normalize(n);
    rec.FrontFace = dot(ray.Direction,rec.WorldNormal) > 0 ? false : true;
    rec.WorldNormal *= 1 - 2 * -(int(rec.FrontFace) - 1);
    rec.HitPoint += rec.WorldNormal*0.0001;
    rec.HitDistance = hit.x;

    return hit.x > t_min && hit.x < t_max;
}

bool hit(const Ray ray, float t_min, float t_max, inout HitPayload rec) {
    HitPayload tempRec;
    bool hitAnything = false;
    float closestObject = t_max;

    for (uint i = 0; i < 1; i++) {
        if (hitSphere(ray, t_min, closestObject, tempRec, world.spheres[i])) {
            hitAnything = true;
            closestObject = tempRec.HitDistance;
            rec = tempRec;
        }
    }

    for (uint i = 0; i < 1; i++) {
        Mesh curMesh = world.meshes[i];
        AABB curAABB = curMesh.Box;
        
        if (curMesh.TriCount == 0)
            break;
        
        if (!hitAABB(ray, curAABB))
            continue;

        for (uint j = 0; j < curMesh.TriCount; j++) {
            Triangle curTri = TBuffer.triangles[curMesh.TriStartingIndex + j];
            
            if (curTri.v0 + curTri.v1 + curTri.v2 == vec3(0))
                break;

            if (hitTriangle(ray, t_min, closestObject, curTri, curAABB, tempRec)) {
                hitAnything = true;
                closestObject = tempRec.HitDistance;
                rec = tempRec;
                rec.MaterialIndex = curMesh.MaterialIndex;
            }
        }
    }

    return hitAnything;
}

#define MAX_BOUNCES 2
vec3 rayColor(Ray ray) {
    HitPayload rec;
    vec3 color = vec3(1);
    vec3 incomingLight = vec3(0);
    float multiplier = 1.0;
    
    for (uint i = 0; i < MAX_BOUNCES; i++) {
        if (!hit(ray, 0.001, INFINITY, rec))
            break;

        Material curMaterial = world.materials[rec.MaterialIndex];

        multiplier *= 0.5;
        ray.Origin = rec.HitPoint;
        float refractionRatio = rec.FrontFace ? (1.0 / curMaterial.IOR) : curMaterial.IOR;
        float cosTheta = min(dot(-ray.Direction, rec.WorldNormal), 1.0);
        float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
        bool cannotRefract = refractionRatio * sinTheta > 1.0;
        vec3 diffuseDir = normalize(rec.WorldNormal + RandomHemisphereDirection(rec.WorldNormal, rngState));
        vec3 specularDir;

        if (cannotRefract || reflectance(cosTheta, refractionRatio) > rand(rngState) || curMaterial.IOR == 0.0) {
            specularDir = reflect(ray.Direction, rec.WorldNormal);
            ray.Direction = mix(specularDir, diffuseDir, curMaterial.Roughness);
        }
        else
            ray.Direction = refract(ray.Direction, rec.WorldNormal, refractionRatio);
        
        vec3 emittedLight = curMaterial.EmissionAlbedo * curMaterial.EmissionStrength;
        incomingLight += emittedLight * color;
        color += multiplier * curMaterial.Albedo;
    }
    
    return incomingLight;
}

vec4 linearToSRGB(vec4 linearRGB) {
    bvec4 cutoff = lessThan(linearRGB, vec4(0.0031308));
    vec4 higher = vec4(1.055)*pow(linearRGB, vec4(1.0 / 2.4)) - vec4(0.055);
    vec4 lower = linearRGB * vec4(12.92);
    return mix(higher, lower, cutoff);
}

#define SAMPLES_PER_PIXEL 1
void main() {
    world.meshes[0].TriStartingIndex = 0;
    world.meshes[0].TriCount = TBuffer.triangles.length();
    world.meshes[0].MaterialIndex = 4;
    world.meshes[0].Box.Min = vec3(-1.0, -1.0, 0);
    world.meshes[0].Box.Max = vec3(1.5, 1.5, 1.5);

    world.spheres[0].Position = vec3(0.0, 1.0, 2.0);
	world.spheres[0].Radius = 0.5;
	world.spheres[0].MaterialIndex = 1;

    world.materials[0].Roughness = 1.0;
    world.materials[0].Albedo = vec3(0.0, 0.0, 1.0);
    world.materials[1].EmissionStrength = 1.0;
    world.materials[1].EmissionAlbedo = vec3(1.0);
    world.materials[2].Roughness = 1.0;
    world.materials[2].Albedo = vec3(1.0, 0.0, 0.0);
    world.materials[3].Roughness = 1.0;
    world.materials[3].Albedo = vec3(0.0, 1.0, 0.0);
    world.materials[3].Roughness = 1.0;
    world.materials[4].Albedo = vec3(1.0);
    world.materials[4].Roughness = 1.0;
    world.materials[5].Albedo = vec3(0.0);
    world.materials[5].Roughness = 1.0;
    world.materials[6].Albedo = vec3(1.0);
    world.materials[6].IOR = 1.5;

    Ray ray;
    ray.Origin = ubo.camPos;

    for (int i = 0; i < SAMPLES_PER_PIXEL; i++) {
        vec2 delta = vec2(i%3-1, i/3-1);
        ray.Direction = vec3(-((in_position.x * float(ubo.screenRes.x)) + delta.x/3) / float(ubo.screenRes.y), (-(in_position.y * float(ubo.screenRes.y)) + delta.y/3) / float(ubo.screenRes.y), -1);
        out_color += vec4(rayColor(ray), 1.0);
    }
    
    out_color /= SAMPLES_PER_PIXEL;
    out_color = linearToSRGB(out_color);
}