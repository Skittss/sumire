#version 450

layout (location = 0) in vec3 fragColor;

layout (location = 0) out vec4 col;

layout(push_constant) uniform Push {
	mat4 transform;
	vec3 colour;
} push;

void main() {
	col = vec4(fragColor, 1.0);
}