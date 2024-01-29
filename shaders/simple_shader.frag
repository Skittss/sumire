#version 450

layout (location = 0) out vec4 col;

layout(push_constant) uniform Push {
	vec2 offset;
	vec3 colour;
} push;

void main() {
	col = vec4(push.colour, 1.0);
}