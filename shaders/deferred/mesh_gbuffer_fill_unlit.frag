#version 450
#extension GL_GOOGLE_include_directive : require


layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUv0;
layout (location = 2) in vec2 inUv1;

layout (location = 0) out vec4 outSwapChainCol;

layout(push_constant) uniform Push {
	layout(offset = 128) int materialIdx;
};

layout(set = 1, binding = 0) uniform sampler2D albedoMap;

// Material properties
#include "../includes/inc_material.glsl"

layout(set = 3, binding = 0) buffer SSBO {
	Material materials[];
};

void main() {
	// Index mesh's material from storage buffer
	Material mat = materials[materialIdx];

	vec2 inUvs[2] = {inUv0, inUv1};

	// Use base colour factors if no albedo map is given
	vec4 albedo = mat.baseColorTexCoord > -1 ? 
		texture(albedoMap, inUvs[mat.baseColorTexCoord]) 
		: vec4(1.0);
	albedo *= mat.baseColorFactors;

	// We do the fragment discard AFTER texture sampling as otherwise we get divergent control flow.
	// More details in a similar issue from a khronos org repository:
	// https://github.com/KhronosGroup/glTF-Sample-Viewer/issues/267
	if (mat.useAlphaMask) {
		if (albedo.a < mat.alphaMaskCutoff) 
			discard;
	}

    // Write directly to swap chain
	outSwapChainCol = albedo;
}