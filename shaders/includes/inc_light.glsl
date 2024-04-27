// Light Types
const int LIGHT_TYPE_PUNCTUAL_POINT = 0;
const int LIGHT_TYPE_PUNCTUAL_SPOT  = 1;
const int LIGHT_TYPE_PUNCTUAL_DIRECTIONAL = 1;

struct Light {
    vec4 color;
    vec3 translation;
    vec3 rotation;
    int type;
    float range;
    float lightAngleScale;
    float lightAngleOffset;
};