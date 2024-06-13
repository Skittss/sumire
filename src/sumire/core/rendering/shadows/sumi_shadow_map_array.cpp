#include <sumire/core/rendering/shadows/sumi_shadow_map_array.hpp>

namespace sumire {

    SumiShadowMapArray::SumiShadowMapArray(
        SumiDevice& device,
        uint32_t shadowmapWidth,
        uint32_t shadowmapHeight
    ) : sumiDevice{ device }, 
        width{ shadowmapWidth }, 
        height{ shadowmapHeight } 
    {

    }



}