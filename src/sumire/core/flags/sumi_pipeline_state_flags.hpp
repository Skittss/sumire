#pragma once

#include <sumire/core/flags/sumi_flags.hpp>

namespace sumire {

    typedef enum SumiPipelineStateFlagBits {
        SUMI_PIPELINE_STATE_DEFAULT = 0x00000000,
        SUMI_PIPELINE_STATE_UNLIT_BIT = 0x00000001,
        SUMI_PIPELINE_STATE_DOUBLE_SIDED_BIT = 0x00000002,
        SUMI_PIPELINE_STATE_HIGHEST = SUMI_PIPELINE_STATE_DOUBLE_SIDED_BIT,
    } SumiPipelineStateFlagBits;

    typedef SumiFlags SumiPipelineStateFlags;

}