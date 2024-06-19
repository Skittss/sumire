#pragma once

namespace sumire {

    typedef enum HQSMdebuggerView {
        HQSM_DEBUG_NONE,
        HQSM_DEBUG_HZB,
        HQSM_DEBUG_LIGHT_COUNT,
        HQSM_DEBUG_LIGHT_CULLING
        //...
    };

    typedef enum HQSMlightCountListSource {
        HQSM_LIGHT_COUNT_LIGHT_MASK,
        HQSM_LIGHT_COUNT_TILE_GROUP_LIGHT_MASK,
        HQSM_LIGHT_COUNT_EARLY_LIST,
        HQSM_LIGHT_COUNT_FINAL_LIST,
        HQSM_LIGHT_COUNT_APPROX_EARLY_DIFFERENCE
    };

}