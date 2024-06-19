#version 450
#extension GL_GOOGLE_include_directive : require

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outCol;

// ---- Input Buffers --------------------------------------------------------------------------------------------

layout(push_constant) uniform Push {
    uvec2 screenResolution;
    uvec2 shadowTileResolution;
    uvec2 tileGroupResolution;
    uvec2 lightMaskResolution;
    uint  listSource; // 0 = light mask, 1 = approx, 2 = early, 3 = final, 4 = approx-early difference.
} push;

#include "../includes/inc_light_mask.glsl"

layout(set = 0, binding = 0) restrict readonly buffer lightMaskBuffer {
    lightMask lightMasks[];
};

layout(set = 0, binding = 1) restrict readonly buffer tileGroupLightMaskBuffer {
    lightMask tileGroupLightMasks[];
};

layout(set = 0, binding = 2) restrict readonly buffer tileLightCountEarly {
    uint earlyTileLightCount[];
};

layout(set = 0, binding = 3) restrict readonly buffer tileLightCountFinal {
    uint finalTileLightCount[];
};

// ---------------------------------------------------------------------------------------------------------------

#include "../../includes/inc_heatmap.glsl"

uint getTileIdx(in uvec2 tileCoord) {
    return tileCoord.x + tileCoord.y * push.shadowTileResolution.x;
}

uint getTileGroupIdx(in uvec2 tileGroupCoord) {
    return tileGroupCoord.x + tileGroupCoord.y * push.tileGroupResolution.x;
}

uint getTileLightMaskIdx(in uvec2 tileLightMaskCoord) {
    return tileLightMaskCoord.x + tileLightMaskCoord.y * push.lightMaskResolution.x;
}

void main() {

    uvec2 pixelCoord = uvec2(vec2(push.screenResolution - uvec2(1)) * inUv) + uvec2(1);
    uvec2 tileCoord          = pixelCoord / 8;
    uvec2 tileGroupCoord     = tileCoord / 8u;
    uvec2 tileLightMaskCoord = tileCoord / 4u;

    uint tileIdx          = getTileIdx(tileCoord);
    uint tileGroupIdx     = getTileGroupIdx(tileGroupCoord);
    uint tileLightMaskIdx = getTileLightMaskIdx(tileLightMaskCoord);

    uint lightMaskCount = 0;
    uint approxCount    = 0;
    uint earlyCount     = earlyTileLightCount[tileIdx];
    uint finalCount     = finalTileLightCount[tileIdx];

    // ---- Light Mask Count Gather -----------------------------------------------------
    uint tileLightMask = lightMasks[tileLightMaskIdx].bits[0];

    while (tileLightMask != 0) {
        uint lightGroupBit = findLSB(tileLightMask);
        uint lightMask     = lightMasks[tileLightMaskIdx].bits[lightGroupBit + 1];
        tileLightMask     ^= (1 << lightGroupBit); // remove processed group

        while (lightMask != 0) {
            uint lightBit   = findLSB(lightMask);
            lightMask      ^= (1 << lightBit);
            lightMaskCount++;
        }
    }

    // ---- Approx Count Gather ---------------------------------------------------------
    uint lightGroupMask = tileGroupLightMasks[tileGroupIdx].bits[0];

//    while (lightGroupMask != 0) {
//        uint lightGroupBit = findLSB(lightGroupMask);
//        uint lightMask     = tileGroupLightMasks[tileGroupIdx].bits[lightGroupBit + 1];
//        lightGroupMask     ^= (1 << lightGroupBit); // remove processed group
//
//        while (lightMask != 0) {
//            uint lightBit   = findLSB(lightMask);
//            lightMask      ^= (1 << lightBit);
//            approxCount++;
//        }
//    }
    // ----------------------------------------------------------------------------------

    vec4 overlayCol = vec4(0.0);
    uint count;

    if (push.listSource == 0) {
        count = lightMaskCount;
    }
    else if (push.listSource == 1) {
        //count = approxCount;
        count = lightGroupMask;
    } 
    else if (push.listSource == 2) {
        count = earlyCount;
    }
    else if (push.listSource == 3) {
        count = finalCount;
    }
    else {
        uint minCount = min(approxCount, earlyCount);
        uint maxCount = max(approxCount, earlyCount);
        count = maxCount - minCount;
        overlayCol = vec4(1.0, 1.0, 1.0, 0.02);
    }

    if (count > 0) {
        overlayCol = vec4(heatMap(count, 1024.0), 0.04);
    }

    outCol = overlayCol;

    //vec2 tileVis      = vec2(tileCoord) / vec2(push.shadowTileResolution);
    //vec2 tileGroupVis = vec2(tileGroupCoord) / vec2(push.tileGroupResolution);

    //outCol = vec4(tileVis, 0.0, 0.05);
}