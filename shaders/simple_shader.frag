#version 450

layout (location = 0) out vec4 col;

layout(push_constant) uniform Push {
	vec3 colour;
	mat4 transform;
} push;

void main() {
	col = vec4(push.colour, 1.0);
}