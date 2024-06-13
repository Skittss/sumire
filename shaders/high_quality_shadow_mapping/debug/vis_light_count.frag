#version 450

layout(push_constant) uniform Push {
    uvec2 screenResolution;
} push;

layout(set = 0, binding = 0) restrict readonly buffer tileLightCountEarly {
    uint earlyTileLightCount[];
};

layout(set = 0, binding = 0) restrict readonly buffer tileLightCountFinal {
    uint finalTileLightCount[];
};

void main() {

}