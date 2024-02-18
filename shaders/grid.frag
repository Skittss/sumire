#version 450

layout(location = 0) in vec3 nearPt;
layout(location = 1) in vec3 farPt;

layout(location = 0) out vec4 col;

layout(set = 0, binding = 1) uniform Camera {
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 projectionViewMatrix;
};

layout(push_constant) uniform GridParams {
    layout(offset = 64) vec3 cameraPos;
    float majorLineThickness;
};

const float TILE_SIZE = 10.0;
const float LINE_THICKNESS = 0.01;

const float FOG_NEAR = 0.01;
const float FOG_FAR = 1.0;

const vec3 GRID_LINE_COLOUR = vec3(0.2);

vec4 gridFragCol(vec3 fragPos, float scale) {
    vec2 coord = fragPos.xz * scale; // use the scale variable to set the distance between the lines
    vec2 dxdz = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / dxdz;
    
    float line = min(grid.x, grid.y);

    float zmin = min(dxdz.y, 1);
    float xmin = min(dxdz.x, 1);
    vec4 color = vec4(GRID_LINE_COLOUR, 1.0 - min(line, 1.0));


    // primary axes
    // z axis
    if(abs(fragPos.x) < 0.1 * xmin)
        color.z = 1.0;

    // x axis
    if(abs(fragPos.z) < 0.1 * zmin)
        color.x = 1.0;

    return color;
}

float calcDepth(vec3 fragPos) {
    vec4 clip_space_pos = projectionMatrix * viewMatrix * vec4(fragPos, 1.0);
    return (clip_space_pos.z / clip_space_pos.w);
}

void main() {

    float t = -nearPt.y / (farPt.y - nearPt.y);
    vec3 planeFragPos = nearPt + t * (farPt - nearPt);

    // compute depth as we mess it up by unprojecting the quad.
    vec4 clipSpacePos = projectionMatrix * viewMatrix * vec4(planeFragPos, 1.0);
    float depth = clipSpacePos.z / clipSpacePos.w;
    gl_FragDepth = depth;
    float clipDepth = depth * 2.0 - 1.0;
    float linearDepth = (2.0 * FOG_NEAR * FOG_FAR) / (FOG_FAR + FOG_NEAR - clipDepth * (FOG_FAR - FOG_NEAR));
    linearDepth /= FOG_FAR; // normalize

    float fog = max(0.0, 0.5 - linearDepth);

    col = gridFragCol(planeFragPos, TILE_SIZE) * step(0.0, t);
    col.a *= fog;
}