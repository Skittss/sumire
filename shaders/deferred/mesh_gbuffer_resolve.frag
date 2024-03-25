#version 450

layout (input_attachment_index = 0, binding = 0) uniform subpassInput gbufferPosition;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput gbufferNormal;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput gbufferAlbedo;

layout (location = 0) in vec2 gbufferUV;

layout (location = 0) out vec4 outCol;

void main() {
    vec3 normal = subpassLoad(gbufferNormal).rgb;
    vec4 albedo = subpassLoad(gbufferAlbedo);
    vec3 position = subpassLoad(gbufferPosition).rgb;
    outCol = subpassLoad(gbufferAlbedo);
}