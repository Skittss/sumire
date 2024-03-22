#version 450

layout (set = 0, binding = 0) uniform sampler2D gbufferPosition;
layout (set = 0, binding = 1) uniform sampler2D gbufferNormal;
layout (set = 0, binding = 2) uniform sampler2D gbufferAlbedo;

layout (location = 0) in vec2 gbufferUV;

layout (location = 0) out vec4 outCol;

void main() {
    vec3 normal = texture(gbufferNormal, gbufferUV).rgb;
    vec4 albedo = texture(gbufferAlbedo, gbufferUV);
    vec3 position = texture(gbufferPosition, gbufferUV).rgb;
    outCol = texture(gbufferAlbedo, gbufferUV);
}