#define M_HALF_PI 1.57079632679

vec3 heatMap(float val, float maxVal) {
    float normalizedVal = val / maxVal;
    float heat = normalizedVal * M_HALF_PI;

    return vec3(
        sin(heat),
        sin(heat * 2.0),
        cos(heat)
    );
}