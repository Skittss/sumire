#pragma once

#include <string>

namespace sumire {

    struct ProfilingBlock {
        uint32_t idx;
        double ms = 0.0;
    };

}