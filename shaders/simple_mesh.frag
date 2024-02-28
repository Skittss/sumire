#version 450

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
	mat4 modelMatrix;
};

// Material Textures
// TODO: These textures are SRGB, so processing them in linear color space requires a conversion
layout(set = 1, binding = 0) uniform sampler2D baseColor;
// layout(set = 1, binding = 1) uniform sampler2D metallicRoughness;
// layout(set = 1, binding = 2) uniform sampler2D normalMap;
// layout(set = 1, binding = 3) uniform sampler2D aoMap;
// layout(set = 1, binding = 4) uniform sampler2D emissiveMap;

// TODO: Material properties

void main() {
	vec3 pointLightDir = ubo.lightPos - fragWorldPos.xyz;
	float dLight = length(pointLightDir);
	float attenuation = ubo.lightIntensity / dLight * dLight;

	vec3 diffuse = attenuation * ubo.lightCol * max(dot(normalize(fragWorldNorm), normalize(pointLightDir)), 0.0);

	vec3 baseCol = texture(baseColor, fragUv).rgb;

	col = vec4((ubo.ambientCol + diffuse) * baseCol, 1.0);
}