#version 450
#extension GL_GOOGLE_include_directive : require

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

layout (set = 1, binding = 0) uniform sampler2D gbufferPosition;
layout (set = 1, binding = 1) uniform sampler2D gbufferNormal;
layout (set = 1, binding = 2) uniform sampler2D gbufferAlbedo;
layout (set = 1, binding = 3) uniform sampler2D gbufferAoMetalRoughEmissive;

#include "../includes/inc_pbr_brdf.glsl"
#include "../includes/inc_light_attenuation.glsl"

void main() {
    // Raw buffer values
    vec4 positionValues            = texture(gbufferPosition, gbufferUV);
    vec4 normalValues              = texture(gbufferNormal, gbufferUV);
    vec4 albedo                    = texture(gbufferAlbedo, gbufferUV);
    vec4 aoMetalRoughEmissiveVales = texture(gbufferAoMetalRoughEmissive, gbufferUV);

    // Unpack
    vec3 position   = positionValues.rgb;
    vec3 normal     = normalValues.rgb;
    float ao        = aoMetalRoughEmissiveVales.r;
    float metallic  = aoMetalRoughEmissiveVales.g;
    float roughness = aoMetalRoughEmissiveVales.b;
    vec3 emissive   = vec3(positionValues.a, normalValues.a, aoMetalRoughEmissiveVales.a);
    
    // === PBR lighting ======================================================================
    //  Using Cook-Torrence BRDF: Lr = Le + (f_d + f_s) * Li * n dot wi

    vec3 f0 = vec3(0.04);
    f0 = mix(f0, albedo.rgb, metallic);

    // roughness is authored as r^2 = a by convention to compensate for non-linear perception.
    float alpha_roughness = roughness * roughness;

    // View vector
    vec3 V = normalize(cameraPosition - position);
    float NdotVplus = max(dot(normal, V), 0.0);

    // Per-light resolve
    vec3 col = vec3(0.0);
    for (int i = 0; i < ubo.nLights; i++) {
        Light light = lights[i];
        vec3 perLightCol = vec3(0.0);
        
        vec3 pToL = light.translation - position;
        // this is faster:
        // float lightDist = 
        float lightDist = length(pToL);

        vec3 L = normalize(pToL);
        vec3 H = normalize(L + V);

        float NdotLplus = max(dot(normal, L), 0.0);
        float NdotHplus = max(dot(normal, H), 0.0);
        float VdotHplus = max(dot(V, H), 0.0);
        if (light.range > 0 && (NdotLplus > 0.0 || NdotHplus > 0.0)) {
            vec3 k_s = BRDF_F_schlick(VdotHplus, f0);
            vec3 k_d = mix(1.0 - k_s, vec3(0.0), metallic); // Metals diminish defuse reflectance
            vec3 f_d = BRDF_lambertian(albedo.rgb);
            vec3 f_s = BRDF_specular_GGX(alpha_roughness, f0, NdotLplus, NdotVplus, NdotHplus, VdotHplus);
            
            // Controlled light attenuation to fade at range.
            float att = 1.0;
            float invSqrRange = 1.0 / (light.range * light.range); // TODO: move to CPU
            att *= attDistanceFrosbite(pToL, invSqrRange, 0.01);

            vec3 Li = light.color.rgb * light.color.a * att;
            col += (k_d * f_d + f_s) * Li * NdotLplus;
        }
    }
    
    outCol = vec4(emissive + col, albedo.a);
}