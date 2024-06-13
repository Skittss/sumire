#version 450

layout (location = 0) out vec2 outUV;

// Expects 3 empty input vertices
void main() {
    outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(outUV * vec2(2.0, -2.0) + vec2(-1.0, 1.0), 0.0, 1.0);
}