#version 450

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outCol;

layout (push_constant) uniform Push {
    uvec2 screenResolution;
    uvec2 shadowTileResolution;
    uint  targetLightIdx;
} push;

void main() {

    vec4 visualiserCol = vec4(0.0);

    // Check if target light is in final light list for tile
    bool lightPresent = true;

    if (lightPresent) {

    } 
    else {
        visualiserCol = vec4(0.6, 0.0, 0.0, 0.5);
    }
    
    // Display visibility

    outCol = visualiserCol;
}