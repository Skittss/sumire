#version 450

layout (location = 0) out vec2 fragOffset;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
	vec3 ambientCol;
	vec3 lightDir;
	vec3 lightPos;
	vec3 lightCol;
	float lightIntensity;
} ubo;

layout(set = 0, binding = 1) uniform Camera {
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 projectionViewMatrix;
};
const vec2 OFFSETS[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, -1.0),
    vec2(1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, 1.0)
);

const float LIGHT_RADIUS = 0.05;

void main() {
    fragOffset = OFFSETS[gl_VertexIndex];

    vec4 lightPosCamera = viewMatrix * vec4(ubo.lightPos, 1.0);
    vec3 vertexPosCamera = lightPosCamera.xyz + LIGHT_RADIUS * vec3(fragOffset, 0.0); // billboard orthogonal to view

    gl_Position = projectionMatrix * vec4(vertexPosCamera, 1.0);
}
