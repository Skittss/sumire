#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec3 nearPt;
layout(location = 1) out vec3 farPt;

layout(set = 0, binding = 1) uniform Camera {
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 projectionViewMatrix;
	vec3 cameraPosition;
};

layout(push_constant) uniform Model {
	mat4 modelMatrix;
};

void main() {

	mat4 invViewProj = inverse(viewMatrix) * inverse(projectionMatrix);

	vec4 near = invViewProj * vec4(pos.x, pos.y, 0.0, 1.0);
	nearPt = near.xyz / near.w;

	vec4 far  = invViewProj * vec4(pos.x, pos.y, 1.0, 1.0);
	farPt  = far.xyz / far.w;

	gl_Position = vec4(pos, 1.0);
}