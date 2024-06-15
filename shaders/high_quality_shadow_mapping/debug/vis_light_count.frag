#version 450

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outCol;

layout(push_constant) uniform Push {
    uvec2 screenResolution;
    uvec2 shadowTileResolution;
} push;

layout(set = 0, binding = 0) restrict readonly buffer tileLightCountEarly {
    uint earlyTileLightCount[];
};

layout(set = 0, binding = 0) restrict readonly buffer tileLightCountFinal {
    uint finalTileLightCount[];
};

uint getTileIdx(in uvec2 tileCoord) {
    return tileCoord.x + tileCoord.y * push.shadowTileResolution.x;
}

void main() {
    uvec2 pixelCoord = uvec2(push.screenResolution * inUv);
    uvec2 tileCoord  = pixelCoord / 32u; // 4x4 tile groups
    uint  tileIdx    = getTileIdx(tileCoord);

    uint tileCountEarly = earlyTileLightCount[tileIdx];
    uint tileCountFinal = finalTileLightCount[tileIdx];

    outCol = vec4(tileCountEarly, 0.0, 0.0, 1.0);
}