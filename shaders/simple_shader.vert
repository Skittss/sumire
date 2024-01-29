#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 col;

layout(push_constant) uniform Push {
	vec3 colour;
	mat4 transform;
} push;

void main() {
	gl_Position = vec4(position, 0.0, 1.0);
}