#pragma once

#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/rendering/lighting/sumi_light.hpp>

/*
* TODO: Implementation of Virtual Shadow Map (VSM).
* 
*  Uses a large sparse virtual texture as an atlas which paginates individual shadow maps.
*  
*/

namespace sumire {
    class SumiVirtualShadowMap {
    public:
        SumiVirtualShadowMap(
            SumiDevice& device,
            uint32_t width, uint32_t height
        );
        ~SumiVirtualShadowMap();

        void generateShadowMaps(std::vector<SumiLight*> lights);

    private:
        void createAtlas();
        const uint32_t atlasWidth;
        const uint32_t atlasHeight;

        SumiDevice& sumiDevice;
    };
}