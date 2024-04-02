#version 450
#extension GL_GOOGLE_include_directive : require

layout (input_attachment_index = 0, binding = 0) uniform subpassInput gbufferPosition;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput gbufferNormal;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput gbufferAlbedo;
layout (input_attachment_index = 3, binding = 3) uniform subpassInput gbufferAoMetalRoughEmissive;

layout (location = 0) in vec2 gbufferUV;

layout (location = 0) out vec4 outCol;

#include "../includes/inc_pbr_brdf.glsl"

void main() {
	// Raw buffer values
	vec4 positionValues = subpassLoad(gbufferPosition);
	vec4 normalValues =  subpassLoad(gbufferNormal);
	vec4 aoMetalRoughEmissiveVales = subpassLoad(gbufferAoMetalRoughEmissive);

	// Unpack
    vec3 position = positionValues.rgb;
    vec3 normal = normalValues.rgb;
    vec4 albedo = subpassLoad(gbufferAlbedo);
	float ao = aoMetalRoughEmissiveVales.r;
	vec2 metalRoughness = aoMetalRoughEmissiveVales.gb;
	vec3 emissive = vec3(positionValues.a, normalValues.a, aoMetalRoughEmissiveVales.a);
	
	// PBR lighting
	//  Using Cook-Torrence BRDF: Lr = (f_d + f_s) * Li * n dot wi
	vec3 f_d = BRDF_lambertian(albedo.rgb);
	// vec3 f_s = ;
	
    outCol = albedo;
}