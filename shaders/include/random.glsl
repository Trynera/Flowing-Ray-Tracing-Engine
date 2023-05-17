uint rngState = uint(uint(abs(in_position.x) * ubo.screenRes.x) * 1973 * 2 + uint(abs(in_position.y) * ubo.screenRes.x) * 9277 * 2) + uint(ubo.time) | 1;

float rand(inout uint state) {
    state = state * 747796405 + 2891336453;
    uint result = ((state >> ((state >> 28) + 4)) ^ state) * 277803737;
    result = (result >> 22) ^ result;
    return result / 4294967295.0;
}

float RandomValueNormalDistribution(inout uint state) {
    float theta = 2 * PI * rand(state);
    float rho = sqrt(-2 * log(rand(state)));
    return rho * cos(theta);
}

vec3 RandomInUnitSphere(inout uint state) {
    float x = RandomValueNormalDistribution(state);
    float y = RandomValueNormalDistribution(state);
    float z = RandomValueNormalDistribution(state);
    return normalize(vec3(x, y, z));
}

vec3 RandomHemisphereDirection(vec3 normal, inout uint state) {
    vec3 dir = RandomInUnitSphere(state);
    return dir * sign(dot(normal, dir));
}