#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 col;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform Push {
	mat4 transform; 
	mat4 modelMatrix;
} push;

const vec3 LIGHT_DIR = normalize(vec3(1.0, -3.0, -1.0));
const vec3 AMBIENT = vec3(0.02);

void main() {
	gl_Position = push.transform * vec4(position, 1.0);

	vec3 normalWorldSpace = normalize(mat3(push.modelMatrix) * normal);

	float intensity = max(dot(normalWorldSpace, LIGHT_DIR), 0.0);

	//fragColor = vec3(uv, 0.0);
	//fragColor = 0.5 + 0.5 * normal;
	fragColor = AMBIENT + col * intensity;
}