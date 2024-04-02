#version 450
#extension GL_GOOGLE_include_directive : require

layout (location = 0) out vec2 fragOffset;
layout (location = 1) out int lightIdx;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
	vec3 ambientCol;
	float nLights;
} ubo;

layout(set = 0, binding = 1) uniform Camera {
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 projectionViewMatrix;
};

#include "includes/inc_light.glsl"

layout(set = 0, binding = 2) buffer LightSSBO {
	Light lights[];
};

const vec2 OFFSETS[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, -1.0),
    vec2(1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, 1.0)
);

const float LIGHT_RADIUS_SCALE = 0.05;

void main() {
    fragOffset = OFFSETS[gl_VertexIndex];
	lightIdx = gl_InstanceIndex;
	Light light = lights[lightIdx];
	
    vec4 lightPosCamera = viewMatrix * light.transform * vec4(0.0, 0.0, 0.0, 1.0);
	float billboardSize = max(0.01, min(0.07, 0.005 * light.range));
    vec3 vertexPosCamera = lightPosCamera.xyz + billboardSize * vec3(fragOffset, 0.0); // billboard orthogonal to view

    gl_Position = projectionMatrix * vec4(vertexPosCamera, 1.0);
}
