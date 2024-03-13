#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec4 joint;
layout(location = 1) in vec4 weight;
layout(location = 2) in vec3 position;
layout(location = 3) in vec3 col;
layout(location = 4) in vec3 normal;
layout(location = 5) in vec4 tangent;
layout(location = 6) in vec2 uv0;
layout(location = 7) in vec2 uv1;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outColor;
layout(location = 2) out vec3 outNorm;
layout(location = 3) out vec3 outTangent;
layout(location = 4) out vec3 outBitangent;
layout(location = 5) out vec2 outUv0;
layout(location = 6) out vec2 outUv1;

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
	int nJoints;
} meshNode;

#include "includes/inc_joint.glsl"

layout(set = 2, binding = 1) buffer jointSSBO {
	Joint jointMatrices[];
};

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
			weight.x * jointMatrices[int(joint.x)].matrix +
			weight.y * jointMatrices[int(joint.y)].matrix +
			weight.z * jointMatrices[int(joint.z)].matrix +
			weight.w * jointMatrices[int(joint.w)].matrix;

		mat4 skinNormalMat = 
			weight.x * jointMatrices[int(joint.x)].normalMatrix +
			weight.y * jointMatrices[int(joint.y)].normalMatrix +
			weight.z * jointMatrices[int(joint.z)].normalMatrix +
			weight.w * jointMatrices[int(joint.w)].normalMatrix;

		mat4 combinedTransform = modelMatrix * meshNode.matrix * skinMat;
		localPos = modelMatrix * meshNode.matrix * skinMat * vec4(position, 1.0);
		mat3 combinedNormalMatrix = mat3(normalMatrix * skinNormalMat);
		outNorm = normalize(combinedNormalMatrix * normal);
		outTangent = normalize(combinedNormalMatrix * tangent.xyz);
	} else {
		localPos = modelMatrix * meshNode.matrix * vec4(position, 1.0);
		outNorm = normalize(mat3(normalMatrix) * normal);
		outTangent = normalize(mat3(normalMatrix) * tangent.xyz);
	}

	// Computer per-vertex bitangent so it is nicely interpolated in fragment shader
	outBitangent = cross(outNorm, outTangent) * -tangent.w;

	localPos /= localPos.w;
	// Standard Camera Projection
	gl_Position = projectionViewMatrix * localPos;

	// Pass remaining vertex attributes to frag shader
	outPos = localPos.xyz / localPos.w;
	outColor = col;
	outUv0 = uv0;
	outUv1 = uv1;
}