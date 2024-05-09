#pragma once

#include <chrono>

namespace sumire {

    struct ProfilingBlockTimestamp {
        std::chrono::steady_clock::time_point start{};
        std::chrono::steady_clock::time_point end{};
    };

}