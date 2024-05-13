#pragma once

#include <vector>

namespace sumire::structs {

    // Combination of Zbin and Ranged Zbin.
    //   Ranged Zbin values offer more conservative estimates for 
    //   light queries over a range (>2) of multiple contiguous Zbins.
    struct zBinData {
        int minLightIdx = -1;
        int maxLightIdx = -1;
        int rangedMinLightIdx = -1;
        int rangedMaxLightIdx = -1;
    };

    struct zBin {
        int minLight = -1;
        int maxLight = -1;
        int firstFullIdx = -1;
        int lastFullIdx = -1;

        std::vector<zBinData> data;

        zBin(const uint32_t size) {
            data = std::vector<zBinData>(size);
        }

        void reset() {
            std::fill(data.begin(), data.end(), zBinData{});
            firstFullIdx = -1;
            lastFullIdx = -1;
            minLight = -1;
            maxLight = -1;
        }
    };

}