#pragma once

#include <sumire/core/materials/sumi_material.hpp>

namespace sumire {

    struct Primitive {
        uint32_t firstIndex;
        uint32_t indexCount;
        uint32_t vertexCount;
        SumiMaterial *material{nullptr};
        uint32_t materialIdx;

        Primitive(
            uint32_t firstIndex, 
            uint32_t indexCount, uint32_t vertexCount, 
            SumiMaterial *material, uint32_t materialIdx
        ) : firstIndex{firstIndex}, indexCount{indexCount}, vertexCount{vertexCount},
            material{material}, materialIdx{materialIdx} {}
    };
    
}