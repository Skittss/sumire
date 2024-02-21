#version 450

layout (location = 0) in vec2 fragOffset;

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

// TODO: Use one triangle instead of quad.
void main() {
    float d = sqrt(dot(fragOffset, fragOffset));
    if (d > 1.0) discard;

    col = vec4(ubo.lightCol, 1.0);
}