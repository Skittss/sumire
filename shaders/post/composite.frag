#version 450

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outCol;

layout(set = 0, binding = 0, rgba16) uniform readonly image2D baseImage;

layout(push_constant) uniform imageInfo {
	vec2 resolution;
};

void main() {
	ivec2 baseImageXY = ivec2(resolution * inUv);
	vec4 baseImageCol = imageLoad(baseImage, baseImageXY);

	outCol = baseImageCol;
}