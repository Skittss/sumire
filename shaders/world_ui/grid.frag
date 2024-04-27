#version 450

layout(location = 0) in vec3 nearPt;
layout(location = 1) in vec3 farPt;

layout(location = 0) out vec4 col;

layout(set = 0, binding = 1) uniform Camera {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 projectionViewMatrix;
    vec3 cameraPosition;
};

layout(set = 1, binding = 0) uniform GridUniforms {
    float opacity;
    float tileSize;
    float fogNear;
    float fogFar;
    vec3 minorLineCol;
    vec3 xCol; 
    vec3 zCol;
};

layout(push_constant) uniform GridParams {
    layout(offset = 64) vec3 cameraPos;
    float majorLineThickness;
};

vec4 gridFragCol(vec3 fragPos, float scale) {
    vec2 coord = fragPos.xz * scale; // use the scale variable to set the distance between the lines
    vec2 dxdz = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / dxdz;
    
    float line = min(grid.x, grid.y);

    float zmin = min(dxdz.y, 1);
    float xmin = min(dxdz.x, 1);
    vec4 color = vec4(minorLineCol, 1.0 - min(line, 1.0));


    // primary axes
    // z axis
    float colAxisThresh = 1.0 / tileSize;
    if(abs(fragPos.x) < colAxisThresh * xmin)
        color.xyz = zCol;

    // x axis
    if(abs(fragPos.z) < colAxisThresh * zmin)
        color.xyz = xCol;

    return color;
}

float calcDepth(vec3 fragPos) {
    vec4 clip_space_pos = projectionMatrix * viewMatrix * vec4(fragPos, 1.0);
    return (clip_space_pos.z / clip_space_pos.w);
}

// TODO: Adjust fog near and far to be more intuitive, and "tile size" should actually 
//       be tile size not subdivisions

void main() {

    float t = -nearPt.y / (farPt.y - nearPt.y);
    vec3 planeFragPos = nearPt + t * (farPt - nearPt);

    // compute depth as we mess it up by unprojecting the quad.
    vec4 clipSpacePos = projectionMatrix * viewMatrix * vec4(planeFragPos, 1.0);
    float depth = clipSpacePos.z / clipSpacePos.w;
    gl_FragDepth = depth;
    float clipDepth = depth * 2.0 - 1.0;
    float linearDepth = (2.0 * fogNear * fogFar) / (fogFar + fogNear - clipDepth * (fogFar - fogNear));
    linearDepth /= fogFar; // normalize

    float fog = max(0.0, 0.5 - linearDepth);

    col = gridFragCol(planeFragPos, tileSize) * step(0.0, t);
    col.a *= opacity * fog;
}