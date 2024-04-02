#version 450
#extension GL_GOOGLE_include_directive : require

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inNorm;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBitangent;
layout (location = 5) in vec2 inUv0;
layout (location = 6) in vec2 inUv1;

layout (location = 0) out vec4 outSwapChainCol;
layout (location = 1) out vec4 outPosition;
layout (location = 2) out vec4 outNormal;
layout (location = 3) out vec4 outAlbedo;
layout (location = 4) out vec4 outAoMetalRoughEmissive;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
	vec3 ambientCol;
	int nLights;
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
#include "../includes/inc_material.glsl"

layout(set = 3, binding = 0) buffer SSBO {
	Material materials[];
};

#include "../includes/srgb2linear.glsl"

void main() {


	// Index mesh's material from storage buffer
	Material mat = materials[materialIdx];

	vec2 inUvs[2] = {inUv0, inUv1};

	// Use base colour factors if no albedo map is given
	vec4 albedo = mat.baseColorTexCoord > -1 ? 
		texture(albedoMap, inUvs[mat.baseColorTexCoord]) 
		: vec4(1.0);
	albedo *= mat.baseColorFactors;

	// PBR properties
	float ao = mat.occlusionTexCoord > -1 ? 
		1.0 + mat.occlusionStrength * (texture(aoMap, inUvs[mat.occlusionTexCoord]).r - 1.0)
		: 1.0;

	//https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_material_pbrmetallicroughness_metallicroughnesstexture
	vec2 metallicRoughness = mat.metallicRoughnessTexCoord > -1 ?
		texture(metallicRoughnessMap, inUvs[mat.metallicRoughnessTexCoord]).bg
		: vec2(1.0, 1.0);
	metallicRoughness *= mat.metallicRoughnessFactors;

	// TODO: Emissive strength extension
	// Emissive textures are passed as RGB but have sRGB values (TODO: This has precision loss)
	//  So need to be converted to linear RGB before using.
	vec3 emissive = mat.emissiveTexCoord > -1 ? 
		srgb2linear(texture(emissiveMap, inUvs[mat.emissiveTexCoord])).rgb
		: vec3(0.0);
	emissive *= mat.emissiveFactors;
	
	// Adjust geometrical properties of back-faces
	vec3 geoTangent = inTangent;
	vec3 geoBitangent = inBitangent;
	vec3 geoNormal = inNorm;
	if (gl_FrontFacing == false) {
		geoTangent *= -1.0; 
		geoBitangent *= -1.0; 
		geoNormal *= -1.0;
	}

	// Normal Mapping
	vec3 normal;
	if (mat.normalTexCoord > -1) {
		// Read tangent space normal from texture
		vec3 Nt = texture(normalMap, inUvs[mat.normalTexCoord]).rgb * 2.0 - 1.0;
		Nt *= vec3(mat.normalScale, mat.normalScale, 1.0); // Apply scale
		Nt = normalize(Nt);

		mat3 TBN = mat3(geoTangent, geoBitangent, geoNormal);
		normal = normalize(TBN * Nt);		
	} else {
		normal = normalize(geoNormal);
	}

	// We do the fragment discard AFTER texture sampling as otherwise we get divergent control flow.
	// More details in a similar issue from a khronos org repository:
	// https://github.com/KhronosGroup/glTF-Sample-Viewer/issues/267
	if (mat.useAlphaMask) {
		if (albedo.a < mat.alphaMaskCutoff) 
			discard;
	}

	// Tightly pack emissive values withing gbuffer to save on RTs:
	//   r: outPosition.a
	//   g: outNormal.a
	//   b: outAoMetalRoughEmissive.a
	outPosition = vec4(inPos, emissive.r);
	outNormal = vec4(normal, emissive.g);
	outAlbedo = albedo;
	outAoMetalRoughEmissive = vec4(ao, metallicRoughness, emissive.b);
}