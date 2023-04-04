#define PI 3.1415926535897932385

uint stepRNG(uint rngState) {
    return rngState * 747796405 + 1;
}

float stepAndOutputRNGFloat(inout uint rngState) {
    rngState = stepRNG(rngState);
    uint word = ((rngState >> ((rngState >> 28) + 4)) ^ rngState) * 277803737;
    word = (word >> 22) ^ word;
    return float(word) / 4294967295.0;
}

uint rngState = uint(600 * in_position.x + in_position.y);

float random() {
    return stepAndOutputRNGFloat(rngState);
}

float random(float min, float max) {
    return min + (max-min)*random();
}

vec3 random_in_unit_sphere() {
    vec3 p = vec3(random(-0.3, 0.3), random(-0.3, 0.3), random(-0.3, 0.3));
    return normalize(p);
}

vec3 random_cosine_direction() {
    float r1 = random();
    float r2 = random();
    float z = sqrt(1-r2);

    float phi = 2*PI*r1;
    float x = cos(phi)*sqrt(r2);
    float y = sin(phi)*sqrt(r2);

    return vec3(x, y, z);
}