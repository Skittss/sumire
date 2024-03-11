#version 450
#extension GL_GOOGLE_include_directive : require

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inNorm;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBitangent;
layout (location = 5) in vec2 inUv;

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
	layout(offset = 128) int materialIdx;
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

	// Normal Mapping
	vec3 normal;
	if (mat.normalTexCoord > -1) {
		// Read tangent space normal from texture
		vec3 Nt = texture(normalMap, inUv).rgb * 2.0 - 1.0;
		Nt *= vec3(mat.normalScale, mat.normalScale, 1.0); // Apply scale
		Nt = normalize(Nt);

		mat3 TBN = mat3(inTangent, inBitangent, inNorm);
		normal = normalize(TBN * Nt);		
	} else {
		normal = normalize(inNorm);
	}

	vec3 pointLightDir = ubo.lightPos - inPos.xyz;
	float dLight = length(pointLightDir);
	float attenuation = ubo.lightIntensity / dLight * dLight;

	vec3 diffuse = attenuation * ubo.lightCol * max(dot(normal, normalize(pointLightDir)), 0.0);

	// outCol = vec4(albedo.rgb, 1.0);

	// Debug Normal Ouput
	// outCol = vec4(0.5 + 0.5 * normal, 1.0);
	// outCol = vec4(0.5 + 0.5 * inNorm, 1.0);
	// outCol = vec4(0.5 + 0.5 * inTangent, 1.0);
	// outCol = vec4(0.5 + 0.5 * inBitangent, 1.0);

	// outCol = pow(outCol, vec4(2.2));

	// outCol = vec4((ubo.ambientCol + diffuse))

	outCol = vec4((ubo.ambientCol + diffuse) * albedo.rgb, 1.0);
}