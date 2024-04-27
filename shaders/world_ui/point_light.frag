#version 450
#extension GL_GOOGLE_include_directive : require

layout (location = 0) in vec2 fragOffset;
layout (location = 1) flat in int lightIdx;

layout (location = 0) out vec4 col;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
    int nLights;
} ubo;

layout(set = 0, binding = 1) uniform Camera {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 projectionViewMatrix;
    vec3 cameraPosition;
};

#include "../includes/inc_light.glsl"

layout(set = 0, binding = 2) buffer LightSSBO {
    Light lights[];
};

// TODO: Use one triangle instead of quad.
void main() {	
    float d = sqrt(dot(fragOffset, fragOffset));
    if (d > 1.0) discard;

    Light light = lights[lightIdx];
    vec3 lightCol = light.color.rgb * light.color.a;
    col = vec4(lightCol, 1.0);
}