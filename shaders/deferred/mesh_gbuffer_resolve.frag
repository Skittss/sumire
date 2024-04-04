#version 450
#extension GL_GOOGLE_include_directive : require

layout (input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput gbufferPosition;
layout (input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput gbufferNormal;
layout (input_attachment_index = 2, set = 1, binding = 2) uniform subpassInput gbufferAlbedo;
layout (input_attachment_index = 3, set = 1, binding = 3) uniform subpassInput gbufferAoMetalRoughEmissive;

layout (location = 0) in vec2 gbufferUV;

layout (location = 0) out vec4 outCol;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
	int nLights;
} ubo;

layout(set = 0, binding = 1) uniform Camera {
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 projectionViewMatrix;
	vec3 cameraPosition;
};

#include "../includes/inc_light.glsl"

layout(set = 0, binding = 2) buffer LightSSBO {
	Light lights[];
};

#include "../includes/inc_pbr_brdf.glsl"

void main() {
	// Raw buffer values
	vec4 positionValues = subpassLoad(gbufferPosition);
	vec4 normalValues =  subpassLoad(gbufferNormal);
    vec4 albedo = subpassLoad(gbufferAlbedo);
	vec4 aoMetalRoughEmissiveVales = subpassLoad(gbufferAoMetalRoughEmissive);

	// Unpack
    vec3 position = positionValues.rgb;
    vec3 normal = normalValues.rgb;
	float ao = aoMetalRoughEmissiveVales.r;
	vec2 metalRoughness = aoMetalRoughEmissiveVales.gb;
	vec3 emissive = vec3(positionValues.a, normalValues.a, aoMetalRoughEmissiveVales.a);
	
	// === PBR lighting ======================================================================
	//  Using Cook-Torrence BRDF: Lr = Le + (f_d + f_s) * Li * n dot wi

	vec3 f0 = vec3(0.04);

	// View vector
	vec3 V = normalize(cameraPosition - position);
	float NdotVplus = max(dot(normal, V), 0.0);

	vec3 col = vec3(0.0);
	for (int i = 0; i < ubo.nLights; i++) {
		Light light = lights[i];
		vec3 perLightCol = vec3(0.0);
		
		vec3 pToL = light.translation - position;
		float lightDist = length(pToL);

		vec3 L = normalize(pToL);
		vec3 H = normalize(L + V);

		float NdotLplus = max(dot(normal, L), 0.0);
		float NdotHplus = max(dot(normal, H), 0.0);
		float VdotHplus = max(dot(V, H), 0.0);
		if (NdotLplus > 0.0 || NdotHplus > 0.0) {
			// roughness is authored as r^2 = a by convention to compensate for non-linear perception.
			float alpha_roughness = metalRoughness.y * metalRoughness.y;
			
			vec3 f_d = BRDF_lambertian(albedo.rgb);
			vec3 f_s = BRDF_specular_GGX(alpha_roughness, f0, NdotLplus, NdotVplus, NdotHplus, VdotHplus);

			vec3 Li = light.color.rgb * light.color.a / (lightDist * lightDist);
			col += (f_d + f_s) * Li * NdotLplus;
		}
	}
	
	outCol = vec4(emissive + col, albedo.a);
}