#version 450
#extension GL_GOOGLE_include_directive : require

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec3 inPos;
layout (location = 2) in vec3 inNorm;
layout (location = 3) in vec2 inUv;

layout (location = 0) out vec4 outCol;

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

layout(push_constant) uniform Push {
	layout(offset = 64) int materialIdx;
};

// Material Textures
// Note: These textures are SRGB, so processing them in linear color space requires a conversion
layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D metallicRoughnessMap;
layout(set = 1, binding = 2) uniform sampler2D normalMap;
layout(set = 1, binding = 3) uniform sampler2D aoMap;
layout(set = 1, binding = 4) uniform sampler2D emissiveMap;

// Material properties
#include "includes/inc_material.glsl"

layout(set = 3, binding = 0) buffer SSBO {
	Material materials[];
};

#include "includes/srgb2linear.glsl"

void main() {
	// Index mesh's material from storage buffer
	Material mat = materials[materialIdx];

	// Use base colour factors if no albedo map is given
	vec4 albedo = mat.baseColorTexCoord > -1 ? texture(albedoMap, inUv) : vec4(1.0);
	albedo *= mat.baseColorFactors;

	// Check if fragment needs to be alpha-masked before doing any other computations
	// TODO: This simple implementation causes z-fighting.
	if (mat.useAlphaMask) {
		if (albedo.a < mat.alphaMaskCutoff) 
			discard;
	}

	vec3 pointLightDir = ubo.lightPos - inPos.xyz;
	float dLight = length(pointLightDir);
	float attenuation = ubo.lightIntensity / dLight * dLight;

	vec3 diffuse = attenuation * ubo.lightCol * max(dot(normalize(inNorm), normalize(pointLightDir)), 0.0);

	outCol = vec4(albedo.rgb, 1.0);
	//outCol = vec4((ubo.ambientCol + diffuse) * albedo.rgb, 1.0);
}