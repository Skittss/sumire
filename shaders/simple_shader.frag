#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragWorldPos;
layout (location = 2) in vec3 fragWorldNorm;

layout (location = 0) out vec4 col;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
	mat4 projectionViewMatrix;
	vec3 ambientCol;
	vec3 lightDir;
	vec3 lightPos;
	vec3 lightCol;
	float lightIntensity;
} ubo;

layout(push_constant) uniform Push {
	mat4 transform; 
	mat4 modelMatrix;
} push;

void main() {
	vec3 pointLightDir = ubo.lightPos - fragWorldPos.xyz;
	float dLight = length(pointLightDir);
	float attenuation = ubo.lightIntensity / dLight * dLight;

	vec3 diffuse = attenuation * ubo.lightCol * max(dot(normalize(fragWorldNorm), normalize(pointLightDir)), 0.0);

	col = vec4((ubo.ambientCol + diffuse) * fragColor, 1.0);
}