#pragma once

#include <string>

namespace kbf {

    struct ProfilingBlock {
        uint32_t idx;
        double ms = 0.0;
    };

}