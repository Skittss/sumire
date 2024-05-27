#include <sumire/core/rendering/shadows/sumi_virtual_shadow_map.hpp>

namespace sumire {

    SumiVirtualShadowMap::SumiVirtualShadowMap(
        SumiDevice& device,
        uint32_t width,
        uint32_t height
    ) : sumiDevice{ device },
        atlasWidth{ width }, 
        atlasHeight{ height } 
    {

    }

    SumiVirtualShadowMap::~SumiVirtualShadowMap() {

    }

    void SumiVirtualShadowMap::generateShadowMaps(std::vector<SumiLight*> lights) {

    }

}