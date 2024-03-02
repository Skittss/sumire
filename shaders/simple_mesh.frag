#version 450
#extension GL_GOOGLE_include_directive : require

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragWorldPos;
layout (location = 2) in vec3 fragWorldNorm;
layout (location = 3) in vec2 fragUv;

layout (location = 0) out vec4 col;

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
// TODO: These textures are SRGB, so processing them in linear color space requires a conversion
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

	Material mat = materials[materialIdx];

	vec3 pointLightDir = ubo.lightPos - fragWorldPos.xyz;
	float dLight = length(pointLightDir);
	float attenuation = ubo.lightIntensity / dLight * dLight;

	vec3 diffuse = attenuation * ubo.lightCol * max(dot(normalize(fragWorldNorm), normalize(pointLightDir)), 0.0);

	vec4 baseCol = texture(albedoMap, fragUv);

	col = vec4((ubo.ambientCol + diffuse) * baseCol.rgb, 1.0);
}