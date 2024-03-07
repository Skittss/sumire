#version 450

layout(location = 0) in vec4 joint;
layout(location = 1) in vec4 weight;
layout(location = 2) in vec3 position;
layout(location = 3) in vec3 col;
layout(location = 4) in vec3 normal;
layout(location = 5) in vec2 uv;

layout(location = 1) out vec3 outPos;
layout(location = 0) out vec3 outColor;
layout(location = 2) out vec3 outNorm;
layout(location = 3) out vec2 outUv;

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

#define MODEL_MAX_JOINTS 512
layout(set = 2, binding = 0) uniform MeshNode {
	mat4 matrix;
	mat4 jointMatrices[MODEL_MAX_JOINTS];
	int nJoints;
} meshNode;

layout(push_constant) uniform Model {
	mat4 modelMatrix;
	mat4 normalMatrix;
};

void main() {

	vec4 localPos;
	// Calculate skinning matrix if mesh has one
	if (meshNode.nJoints > 0) {
		// Calculated as per glTF 2.0 reference guide
		mat4 skinMat = 
			weight.x * meshNode.jointMatrices[int(joint.x)] +
			weight.y * meshNode.jointMatrices[int(joint.y)] +
			weight.z * meshNode.jointMatrices[int(joint.z)] +
			weight.w * meshNode.jointMatrices[int(joint.w)];
		localPos = modelMatrix * meshNode.matrix * skinMat * vec4(position, 1.0);
		outNorm = normalize(transpose(inverse(mat3(modelMatrix * meshNode.matrix * skinMat))) * normal);
	} else {
		localPos = modelMatrix * meshNode.matrix * vec4(position, 1.0);
		outNorm = normalize(transpose(inverse(mat3(modelMatrix * meshNode.matrix))) * normal);
	}

	// outNorm = normalize(mat3(normalMatrix) *  outNorm);

	// Standard Camera Projection
	gl_Position = projectionViewMatrix * localPos;

	// Pass remaining vertex attributes to frag shader
	outPos = localPos.xyz / localPos.w;
	outColor = col;
	outUv = uv;
}