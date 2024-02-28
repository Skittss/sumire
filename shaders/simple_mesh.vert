#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 col;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 1) out vec3 fragWorldPos;
layout(location = 0) out vec3 fragColor;
layout(location = 2) out vec3 fragWorldNorm;
layout(location = 3) out vec2 fragUv;

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

layout(set = 2, binding = 0) uniform MeshNode {
	mat4 matrix;
} meshNode;

layout(push_constant) uniform Model {
	mat4 modelMatrix;
};

void main() {
	vec4 vertexWorldPos = modelMatrix * meshNode.matrix * vec4(position, 1.0);
	gl_Position = projectionViewMatrix * vertexWorldPos;

	fragWorldNorm = normalize(mat3(modelMatrix) * normal);
	fragWorldPos = vertexWorldPos.xyz;
	fragColor = col;
	fragUv = uv;
}