#pragma once

/*
* 
* TODO: Implementation of shadow maps as a fixed-dimensions texture array.
* 
* Less flexbile than using a VSM or shadow atlas, but far more simple to implement and use.
* 
* A potential solution for flexibility is to make multiple of these texture arrays for different levels of shadow detail.
* 
*/

#include <sumire/core/graphics_pipeline/sumi_device.hpp>

namespace sumire {

    class SumiShadowMapArray {
    public:
        SumiShadowMapArray(
            SumiDevice& device, 
            uint32_t shadowmapWidth, 
            uint32_t shadowmapHeight
        );
    private:
        SumiDevice& sumiDevice;

        const uint32_t width;
        const uint32_t height;
    };

}