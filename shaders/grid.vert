#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec2 uvOut;

layout(set = 0, binding = 1) uniform Camera {
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 projectionViewMatrix;
};

layout(push_constant) uniform Model {
	mat4 modelMatrix;
};

void main() {
    uvOut = uv;
    gl_Position = projectionViewMatrix * modelMatrix * vec4(pos, 1.0);
}