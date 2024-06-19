#version 450

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outCol;

// ---- Input Buffers --------------------------------------------------------------------------------------------

layout(push_constant) uniform Push {
    uvec2 screenResolution;
    float minColRange;
    float maxColRange;
} push;

layout(set = 0, binding = 0) uniform sampler2D minMaxHzb;

void main() {
    vec2 nearFar = texture(minMaxHzb, inUv).rg;

    vec3 overlayCol = vec3(nearFar, 0.0);
    outCol = vec4(overlayCol, 1.0);
}