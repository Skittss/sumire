#version 450

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 col;

layout(push_constant) uniform GridParams {
	vec2 gridOrigin;
    float majorLineThickness;
};

void main() {
    col = vec4(uv, 1.0, 1.0);
}